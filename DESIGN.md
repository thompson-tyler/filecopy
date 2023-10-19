---
header-includes:
  - \usepackage{setspace}
---

# Report for Remote File Copy

### CS117 Jacob Zimmerman and Tyler Thompson

# Introduction

There will be several isolated components,
all of which are, to the best of our ability,
the bare minimum of cleverness that they need
to be.

A major inspiration for this design was a quotation
from Joe Armstrong who developed the Erlang BEAM machine:

_‘It was during this conference that we realised that the work we were doing on Erlang was very different from a lot of mainstream work in telecommunications programming. Our major concern at the time was with detecting and recovering from errors. I remember Mike, Robert and I having great fun asking the same question over and over again: "what happens if it fails?" -- the answer we got was almost always a variant on "our model assumes no failures." We seemed to be the only people in the world designing a system that could recover from software failures.’_

Rather than build an intricate, robust system to prevent failure,
the goal of this system is to have everything be simple enough that failures can be **self-healing**.
Allowing us to prioritize designs that are _stupid_ rather than _smart_.

In our system _smartness_ is an optimization.

Because of this, our system is inherently agnostic to chronology.
Files could be tranferred concurrently. In fact
**both** the client and server processes can be killed and restarted
at any point in a single file transfer, and the protocol is able to heal and
succeed.\*

We spent a whole week devising this system before writing any code, and once
we had all our stupid `C++` bugs sorted, it worked pretty much first time.

\*Adding this feature to our initial design took only 4 extra lines of code,
the power of _stupidity_.

### Successful cases

In its current implementation, this program can reliably transfer files at all levels of network and disk nastiness. It correctly transfers the provided `SRC` directory in about 3 minutes at full nastiness. It also correctly transfers binary files. In fact, we transferred the `fileclient` executable itself and it ran perfectly! *Now to make it into a worm...*

#### Some terminology

- **Smart** vs **stupid** is another way of saying selfconscious vs unselfconscious
- A **message** is a requested action that is conveyed over a network
- A **id** is a number that we associate with a filename
- A **module** is a set of related functions

# Modules

## The `packet` module

This module defines the structs representing packets as defined below, as well
as a number of functions used to easily construct those packets. All packets are
easily sent over the network because they are implemented as tagged unions
which contain no pointers, therefore they are serialized and deserialized
easily via C-style casts.

## The `files` module

Provides functions to reliably read files from disk and a data structure
`files_t` which stores metadata of the files on disk — it doesn't store file contents.

This module uses a nasty file pointer under the hood, but provides the user with reliable file reading and writing. Reliable file reading is achieved with the following process:

1. Split the file into chunks of `N` bytes, where `N` is chosen randomly in the range `[900, 1100]`
2. For each chunk, read and hash the chunk repeatedly until the same hash appears at least `M` times
3. When a hash appears a sufficient number of times, return the corresponding read
4. Repeat for all chunks

**Why are files reads split into chunks?** Files are read in chunks because it was observed that large reads were far more prone to corruption than smaller reads. Reads of about 1000 bytes seemed to be quite reliable (~80% uncorrupted rate) while maintaining quick file reads overall.

**Why are chunk sizes randomly chosen?** While the repeat hash process is good, it does not provide 100% accurate reads. Also, it was observed that corruption on a file often occurs in the same way (eg. the fourth byte is overwritten with a `'.'`). Because of this, it is possible that two independent file reads generate the same corrupted data. To essentially eliminate the odds of this occurring, chunk sizes are randomized so that corruption is also randomized, making a "corruption collision" nearly impossible.

**How is `M` chosen?** Since lower nastiness levels have significantly lower corruption rates, `M` is set based on the disk nastiness setting. This allows lower nastiness settings to read much quicker. In the implementation it was made to be `M = 3 * disk_nastiness`.

If `M` matching hashes are not achieved by `4 * M` reads, the read will fail declare an unfixable disk error, which will trigger an SOS.

The `files` module uses this reliable read mechanism to convert a file into the all the necessary packets to transfer the file, including correct checksum.

To achieve reliable writes, data is first written to disk, then read using the reliable reader, and compared to the original data. If it matches, then the data was successfully written. Otherwise, the process is repeated until successful, or a repeat threshold is hit at which point an SOS is triggered.

The reliability mechanisms of this module are a sort of personal end-to-end check just for disk IO. That being said, they are entirely separate from the real end-to-end check performed later, and are no more than an optimization ahead of resending. We want to stress this point, ***the complexity of this module is an optimization, not a feature of the protocol. If this module just returned the result of the first nasty read, then the protocol would still eventually succeed, albeit much slower.***

## The `messenger` module

Contains functions to reliably send packets over the network or send a
collection of files to the server. A `messenger_t` struct must be initialized
with a "nasty socket" and the nastiness used for that socket. The messenger
functions will adapt to the nastiness level and allow for a greater number of
retries under higher nastiness levels. It also contains a monotonically
increasing (though not necessarily linear), sequence counter used to order
each outgoing packet.

Sending via a `messenger` _guarantees_ that the receiver has responded,
and is aware of the message.

The module has functions to send either an array of packets or a `files_t`
instance containing files to send. The file sending function utilizes the packet
sending function, but first splits the files up and sends the whole sequence of
packets necessary to fully transfer each file.

As long as the protocol is followed, multiple files could be transferred
concurrently, though we don't actually do this.

Both packet sending and file sending give a best effort attempt, and fail after
a certain number of retries, the number of retries is adaptive to
network `nastiness`. In practice, this limit has only been hit when the server isn't actually running.

After a file is sent, this module also performs an end-to-end check for that
file by hashing it and asking the server to compare the hashes. If the check
fails, then the transfer is retried until it succeeds, or retry threshold is
hit.

If a message that is sent is responded with via an `SOS`, the transfer of that
file restarts.

## The `cache` module

This module takes inspiration from walls (which are underappreciated).
It provides one real function called `bounce` that takes in a packet,
tries to perform the requested action, and returns a new packet `ACK` if
the action was successful, and `SOS` otherwise.

This module is meant to be used entirely by the server, and is used to interpret
and act on incoming packets. It provides a number of idempotent functions which
correspond to the actions requested by various packet types.
These functions are designed to be
called multiple times safely so that duplicate packets have no adverse effect.
This is achieved with a combination of checking sequence numbers and checking
file status.

Based on inferring the `cache`'s responses, a client can estimate the current
state of the server. The server does not know it's own state, and is therefore
unselfconscious and **stupid**.

There are internal optimizations to prevent repeated work that do not violate
the idempotence of each action.

### The `listen` function

`listen` is the main server function, it is a loop that does the following:

- read a packet
- `bounce` the packet against the cache
- respond with the same packet metadata plus a copy of the highest recorded sequence
  number that the server has received\*

\* This is used as part of the self healing for recovering from and old process

# The Protocol

A big theme of this protocol is to make the server very simple and letting the
client handle most of the intelligence. There are 7 packet types which work
together to transfer files:

- `SOS`
- `ACK`
- `PREPARE`
- `SECTION`
- `CHECK`
- `KEEP`
- `DELETE`

## Process

The protocol works in 4 distinct steps:

### 1. Prepare for file

When the client wishes to transfer a new file, it sends a `PREPARE` packet containing the filename, file length, and the number of parts (or "sections") the file will be broken into. Since each packet has a maximum size of around 500 bytes, some of the larger files we tested need to be broken into over 6000 sections. The client then waits for the server to respond with an `ACK` before moving on to sending sections.

After receiving a `PREPARE` packet, the server will add a new file entry into its cache with the metadata sent over by the client. The server keeps track of the sequence number the `PREPARE` message was sent with so that it can filter out packets from older failed transfers. Since the server's actions are idempotent, if it received the same `PREPARE` packet more than once, it will `ACK` the duplicates but do nothing.

### 2. Send sections

Once the client has received an `ACK` for the `PREPARE`, it sends a barrage of `SECTION` packets, each a different bit of the file being sent. It waits to receive `ACK`s for these, and removes the acknowledged ones from its queue.

When the server receives a `SECTION`, it ensures that is has a cache entry for the corresponding file. If it does, then it checks whether it has received that section before, and saves it if it's a new section, then responds with an `ACK`. This ensures that duplicate `SECTION`s are still ACK'd but have the same effect, giving the property of idempotence. If the server did not recognize the file that the section was for (i.e. it did not receive a `PREPARE`), then it sends an `SOS` message back to the client, indicating that it needs to restart the transfer process.

### 3. Checking (end-to-end)

After sending all the sections of a file and receiving ACKs for each, the client sends the hash for the entire file to the server in a `CHECK` packet. If the client then receives an `ACK`, then it knows the file is good, and if it receives an `SOS` then it knows the file transfer did not succeed and it restarts the transfer for that file.

When the server receives a `CHECK` packet, it checks the following:

1. That it has a file entry for the requested file
2. That is has received all sections for the file
3. That the server-side data hashes to the correct hash

If any of these checks fail, then the server responds with `SOS` which triggers a restart of the transfer. Otherwise, the server keeps the file on disk (marked as TMP) and responds with an `ACK`. TMP is short for temporary; the server saves files with the extension `.TMP` since the file *could* still be deleted in the next step. If the file was already previously checked, then the server does nothing and responds with the result of that previous check, either a successful `ACK` or a failing `SOS`.

Note that the third check — checking that server's data hashes correctly — occurs *after* saving the temp file to disk. In this way, the third check is the core of the end-to-end check, since if that step succeeds then we're guaranteed that the correct data has been transferred to the server's disk.

### 4. Keep or Delete

Once a file is checked, the client will either send a `KEEP` or `DELETE` packet, which both tell the server what to do with the TMP file created in step 3.

When receiving a `KEEP` packet, the server checks whether it has a TMP file for that id. If it does then it removes the TMP indicator which cements the file as completed, and responds with `ACK`. If it does not have the indicated TMP file, it responds with `SOS` indicating that something has gone horribly wrong.

If the client hears an `ACK`, then the file transfer process is complete! Huzzah! Though if it receives an `SOS` then the client restarts the whole process at step 1.

Why does the `DELETE` packet exist? That's an excellent question. `DELETE` gives the client freedom to roll back if it finds something has gone wrong after the check step. Though in the current implementation this never happens. Removing this step and assuming `KEEP` was discussed but in the spirit of giving the client agency, it remains.

## Packet Structure

\small

```
| i32 type | HEADER  | DATA    |
| -------- | ------- | ------- |
| SOS      | i32 len | i32 seq | i32 id | i32 maxseq      |                 |              |
| ACK      | i32 len | i32 seq | i32 id | i32 maxseq      |                 |              |
| PREPARE  | i32 len | i32 seq | i32 id | i8[80] filename | i32 file length | i32 nparts   |
| SECTION  | i32 len | i32 seq | i32 id | i32 partno      | i32 offset      | u8[488] data |
| CHECK    | i32 len | i32 seq | i32 id | u8[20] checksum | i8[80] filename |              |
| KEEP     | i32 len | i32 seq | i32 id |                 |                 |              |
| DELETE   | i32 len | i32 seq | i32 id |                 |                 |              |
```

\normalsize

- `len` is the length of the entire packet in bytes

- `seq` is the sequence number assigned by the `Messenger`, and newer packets
  must have higher sequence numbers than older packets

- `id` is a unique identifier assigned to each file, not each packet. They are
  assigned sequentially but it doesn't matter how they're assigned, they only
  need to be unique

- `SOS` is constructed by the server as a response to a previous packet. SOS
  packets are constructed by sending the incoming packet back to the client with
  the type value changed to `SOS`. SOS packets trigger the client to reset
  whatever process prompted the SOS - this is the self healing mechanism. An
  `SOS` has a matching `seqno` to the packet that triggered it.

- `ACK` is constructed the same as SOS. It notifies the client that the
  requested action with a matching `seqno` was performed successfully. The
  client will continue resending packets until it receives an ACK for them.

- `PREPARE` is sent to indicate to the server to get ready for a file separated
  into `nparts` sections, totalling `filelength` bytes. The server will
  associate the `filename` with the `id` and be capable of receiving `SECTION`
  packets to reconstruct the indicated file.

- `SECTION` is a section of a file, identified by its `partno` and placed in
  buffer at `offset`. The server expects that it had previously received a
  `PREPARE` packet for the indicated `id`.

- `CHECK` tells the server that an end to end check for a specific file is
  necessary. The server will ensure it received all the file's sections, then
  hash the file and compare it to `checksum`. Both `filename` and `id` are
  provided so that the client can also request a `CHECK` for a file that the
  target directory contained a priori. An ACK is sent if the file is correct
  and stored in a `.tmp` file. SOS
  is sent if the file is damaged, triggering a retransmission of that file.

- `KEEP` tells the server to save the `.tmp` file matching an `id` to disk

- `DELETE` tells the server to delete the `.tmp` file matching an `id`

It is always assumed that these messages come in any order and may be duplicated.

The `Packet` data structure is represented as a `struct` and written to be
easily serializable.

## When Things Go Wrong

This entire protocol has been designed so that each step should succeed nearly 100% of the time. This is achieved with reliability tools such as the `files` module — which provides reliable disk reads — and the `messenger` module — which provides reliable packet transfer. However, the protocol still treats each step as if it could fail often. Each step is allowed to trigger an `SOS`, which triggers the client to go back and restart the transfer process. The beauty of this is that the reliability tools mentioned are only *optimizations*. In fact, the protocol would eventually succeed even if both the disk reads and packet transfer tools were highly unreliable, and still guarantees correctness because of the check (end-to-end) step.

The challenge of dealing with duplicate packets was solved by making all of the server "actions" idempotent. For example, if the server receives a duplicate `SECTION` packet, then it *still* responds with an `ACK` but takes no further action. This also solves the case when an `ACK` is dropped and the client re-sends a section. This design choice hugely simplifies the protocol since far less state needs to be maintained between the client and the server. The client keeps tracks of where it's at in the process, and the server simply acts.

## Gradelogs

We included all the gradelogs that were recommended by the spec,

- Client
  - File: <name>, beginning transmission, attempt <attempt>
  - File: <name> transmission complete, waiting for end-to-end check, attempt <attempt>
  - File: <name> end-to-end check succeeded
  - File: <name> end-to-end check failed
- Server
  - File: <name> starting to receive file
  - File: <name> received, beginning end-to-end check
  - File: <name> end-to-end check succeeded
  - File: <name> end-to-end check failed

On the client side, we also log when `ACK`s aren't received for packets, and it's resending them.

The `files` module also logs when writing a file to disk produces a corrupted file, and if the writing process fails overall it logs that as well. When a file is being read, the `files` module waits until the end of the reading process and logs how many corrupted reads it found.
