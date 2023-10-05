(*
  HIGH-LEVEL PROCESS
  
  For each file in SRC
    - client tells server that a check is necessary
    - server performs check and responds to the client with either FAIL or SUCC
    - client confirms to server server it knows about FAIL or SUCC
    - server acknowledges client, 
      sending either timeout or responding to the client back with client's status,
      if client receives timeout resend the confirmation.
*)

exception Unimplemented
exception NetworkError
exception NeedsResend
exception WaitForAnotherMessage

type clientmessage = CheckIsNecessary of { filename : string; filehash : int }

type servermessage =
  | PleaseCheck of { filename : string; checksum : int }
  | FileGood of { filename : string }
  | FileBad of { filename : string }

type packet = header * message
and header = { checksum : int; time_sent : int; uuid : int; size : int }
and message = Client of clientmessage | Server of servermessage | Timeout

let extract_clientmsg = function
  | hdr, Client msg -> (hdr, msg)
  | _ -> raise WaitForAnotherMessage

let extract_servermsg = function
  | hdr, Server msg -> (hdr, msg)
  | _ -> raise WaitForAnotherMessage

(* theoretically wrappers around the c150class *)
let read_blocking () : packet = raise Unimplemented
let write_nonblocking (packet : packet) : unit = raise Unimplemented
let idcount = ref (-1)

let hdr_checksum_good packet =
  let hdr, _ = packet in
  (* TODO: *)
  hdr.checksum = hdr.checksum

let mk_fresh_packet msg =
  idcount := !idcount + 1;
  ({ uuid = !idcount; checksum = 0; time_sent = 0; size = 0 }, msg)

let mk_client_packet msg = mk_fresh_packet (Client msg)
let mk_server_packet msg = mk_fresh_packet (Server msg)

(* sends a packet,
   returns the first valid packet that matches `unpack_response_packet` *)
let send_and_receive packet (unpack_response_packet : packet -> 'a option) =
  let rec send_and_receive_rec attempt_no unpack_fn to_send_packet =
    (* just changing the variable name *)
    write_nonblocking to_send_packet;
    let sent_packet = to_send_packet in

    (* utils *)
    let is_fresh_packet sent recv = (fst sent).uuid < (fst recv).uuid in
    let max_retries = 6 in

    let rec get_valid_response () =
      match read_blocking () with
      (* can ignore header since these are just abstractions *)
      | _, Timeout when attempt_no > max_retries -> raise NetworkError
      | _, Timeout ->
          (* freshen the header and resend the sent packet data *)
          send_and_receive_rec (attempt_no + 1) unpack_fn
            (mk_fresh_packet (snd sent_packet))
      (* got a valid message *)
      | (recv_hdr, recv_msg) as recv_packet
        when hdr_checksum_good recv_packet
             && is_fresh_packet sent_packet recv_packet -> (
          match unpack_fn recv_packet with
          | Some expected_response_data -> expected_response_data
          | None -> get_valid_response ())
      (* read more packets, might have to read until timeout *)
      (* TODO: What if bad packets keep spamming after good packet dropped?
               How can it know to resend? *)
      | _ -> get_valid_response ()
    in
    get_valid_response ()
  in
  send_and_receive_rec 0 unpack_response_packet packet

(* useful for server just starting out,
   might want to use a while loop instead of recursion here *)
let rec sit_and_listen_until can_extract_expected_response_packet =
  match read_blocking () with
  | _, Timeout -> sit_and_listen_until can_extract_expected_response_packet
  | recv_packet when hdr_checksum_good recv_packet -> (
      match can_extract_expected_response_packet recv_packet with
      | Some expected_response_data -> expected_response_data
      | None -> sit_and_listen_until can_extract_expected_response_packet)
  | _ -> sit_and_listen_until can_extract_expected_response_packet

let hash_file filename = raise Unimplemented

(* still need a better way of representing current state *)
let server_e2e attempts : bool =
  let filename, filehash =
    sit_and_listen_until (function
      | _, Client (CheckIsNecessary { filename; filehash }) ->
          Some (filename, filehash)
      | _ -> None)
  in
  true

(* TODO, do the rest of the work *)
let client_e2e filename filehash attempts : bool =
  let message =
    send_and_receive
      (mk_client_packet (CheckIsNecessary { filename; filehash }))
      (* expected response is either FileGood or FileBad *)
        (function
        | _, Server (FileGood _ as message) | _, Server (FileBad _ as message)
          ->
            Some message
        | _ -> None)
  in
  match message with FileGood _ -> true | _ -> false
