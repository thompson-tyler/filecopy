# Design Document for Remote File Copy

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
the goal of this system is to have everything be simple enough that failures can be self-healing.
Allowing us to prioritize objects that are _stupid_ rather than _smart_.

Any _smartness_ is an optimization.

#### Some terminology

- A **message** is a requested action that is conveyed over a network
- A **method** is a function on a C++ object.
  Though our objects use a _message passing_ style,
  we use the term method to avoid confusion.
- A **id** is a number that we associate with a filename
- An **object** is a C++ object
- A **module** is a set of related functions

# The protocol

### Packet Structure

```
| HEADER                                | DATA
| i32 type | i32 len | i32 seq | i32 id |
| -------- | ------- | ------- | ------ | --------------- | --------------- | ------------ |
| SOS      | i32 len | i32 seq | i32 id |                 |                 |              |
| ACK      | i32 len | i32 seq | i32 id |                 |                 |              |
| PREPARE  | i32 len | i32 seq | i32 id | i8[80] filename | i32 filelength  | i32 nparts   |
| SECTION  | i32 len | i32 seq | i32 id | i32 partno      | i32 offset      | u8[488] data |
| CHECK    | i32 len | i32 seq | i32 id | u8[20] checksum | i8[80] filename |              |
| KEEP     | i32 len | i32 seq | i32 id |                 |                 |              |
| DELETE   | i32 len | i32 seq | i32 id |                 |                 |              |
```

- `len` is the length of the entire packet in bytes 

- `seq` is the sequence number assigned by the `Messenger`, and newer packets
  must have higher sequence numbers than older packets 

- `id` is a unique identifier assigned to each file, not each packet. They are
  assigned sequentially but it doesn't matter how they're assigned, they only
  need to be unique

- `SOS` is constructed by the server as a response to a previous packet. SOS
  packets are constructed by sending the incoming packet back to the client with
  the type value changed to `SOS`. SOS packets trigger the client to reset
  whatever process prompted the SOS - this is the self healing mechanism.

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
  target directory contained a priori. An ACK is sent if the file is intact. SOS
  is sent if the file is damaged, triggering a retransmission of that file.

- `KEEP` tells the server to save the file matching an `id` to disk

- `DELETE` tells the server to delete the file matching an `id`

It is always assumed that these messages come in any order and may be duplicated.

The `Packet` data structure is represented as a `struct` and written to be
easily serializable.

# Objects and Modules

### The `diskio` Module

There are two utility functions here for remedying the nastyfile hard drive
failures `fileToBuffer` and `bufferToFile` which provide reading and writing
respectively. These functions use brute force to guarantee a correct fileread.

Given a preset parameter `MAX_DISK_FAILURES`:

**To read**

- Create a list of checksums `MAX_DISK_FAILURES` in length
- Repeatedly read the file and store the hashcode
- Compare the checksum with previous attempts
- When two checksums match we assume the file contents to be correct.

**To write**

- Hash the data into a checksum
- Try to write
- Reread what we wrote, using the function above to guarantee safe read
  then verify the checksum we read matches the checksum from the client

**Assumptions**

This assumes that the odds of an identical disk error is less than
`2 / MAX_RETRY_ATTEMPTS`

### The `ClientManager` Object

A `ClientManager` is constructed from a directory and a `NASTYFILE` handler.
Upon creation it stores metadata about all the files in the directory that
it has to send, giving them each an id.

It stores an association map between an id

- filename
- file contents
- file transfer status `LOCALONLY | EXISTSREMOTE | COMPLETED`
- number of previous SOS failures with this file

It has two methods, `sendFiles` and `endToEndCheck`, both of which take a
messenger object to ease communication.

When the server gives an SOS the

### The `Messenger` Object

The `Messenger` module implements network communication from the
client side.

It has one "real" function, `send`, which:

1. Takes in a list of messages
1. Assign them each a monotonically increasing sequence number
1. Store all the messages in a table indexed by their sequence number.
1. Loop through the table, sending each message
1. Read all messages until a timeout
   - If receives a packet with immature sequence number, drop it
   - Otherwise if it receives an ACK, remove the message from the table
     with corresponding sequence number
   - If it's is an SOS, abort and return `false`
1. Repeat until there are no unanswered messages left in the table

This guarantees that

- old responses are ignored
- the server definitely knows what client wants it to know

### The `FileCache` Object

The `FileCache` is responsible for all the _thinking_ that the server will do.
This isn't too much thinking at all.

All of its functions return `true`, to signify an expected behaviour (ACK),
and `false` to signify unexpected behaviour (SOS).

It's responses **never** change based on incoming sequence number, if an old
message is sent — it **will** be answered. It is always the client's responsibility
to handle old messages.

The interface provides five methods:

`idempotentCheckFile`

`idempotentSaveFile`

`idempotentDeleteFile`

`idempotentPrepareForFile`

`idempotentStoreFileChunk`

I won't belabor my point by explaining what they do.
But these are all the actions a client can tell the server to do.

The only _thinking_ the `FileCache` does is manage which operations
would be redundant, sometimes based on the sequence number.

It also indicates SOS unsparingly,
often due to old incoming messages, these indicate to the client to start over with that file.
As far as the filecache is concerned, any other behaviour is an optimization.

However, if something can't be done, the FileCache will notify it's caller
by returning `false`.

To see how it works, check out the idempotence section later

### The `Responder` Object

The responder has one function, `bounce`.

`bounce` receives a packet and returns a packet. Always Always.

It is meant to resemble a wall, doing the following:

1. Convert packet to a message
1. Given `ACK` or `SOS` (not sure why) returns them as is
1. Otherwise hand a `Filecache` an idempotent action based on the message
1. If the filecache says it was unexpected, return `SOS` otherwise
   return `ACK`
1. Ensure that the returned packet has the same sequence number and id as the incoming

There are no other cases, it doesn't care about old, new or state, that's it.
Pretty dumb, and we like it that way :)

### `Packet` and `Message` Objects

Just some pretty print functionality and accessors.

Internally `Message` uses a discriminated union pattern to store
variants, like an SML datatype. It also has some safety checks

### `fileclient` and `fileserver` Modules

These have the main function and are responsible
for allocating `NASTYFILE` handlers and `NastySocket`s.

# Idempotence

erm... there is kind of a lot to explain here, not sure if I'll run through every case in detail

# Self-healing

When the `ClientManager` receives an SOS for a file, it will eventually just
try again. Sure this isn't the _most_ powerful self-healing mechanism, but it
will be good enough for now.

Importantly though, it gets called often—anytime ANYTHING weird happens. This
means that the client can make optimizations on top of this, but as long as
the semantic meaning of SOS is recognized, the server can be very simple, and
a slightly clever client can manage to ignore most calls to restart.
