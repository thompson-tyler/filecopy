(* data checksum *)

(* LEGACY

type servermessage =
  | HereIsABlob of { uuid : int; n_parts : int; data : string }
  | HereIsASection of {
      data : string;
      len : int;
      part_number : int;
      uuid : int;
    }
  | HeyDidUGetIt of { uuid : int; part_number : int }
  | Done

and clientmessage =
  | GiveMeNextBlob
  | SoundsGoodToMe
  | SectionLooksGood of { uuid : int; part_number : int }
  | SectionLooksWeird of { uuid : int; part_number : int }
  | PleaseResendBlob of { uuid : int }
  | Done

type packet = checksum * size * message
and checksum = int
and size = int
and message = Client of clientmessage | Server of servermessage 
*)

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

(* End to end check *)
type clientmessage =
  | TimeForCheck of { filename: string }
  | FileGood of { filename: string }
  | FileBad of { filename: string }
  | Timeout

type servermessage =
  | PleaseCheck of { filename: string; checksum: int }
  | Timeout

(* tries to read from sockets, times out if takes too long *)
let server_read () : clientmessage = raise Unimplemented

(* writes message to socket, doesn't block *)
let server_write (msg : servermessage) = raise Unimplemented


let server_processmsg attempts message = function
  | TimeForCheck filename -> filename
  | FileGood filename -> filename
  | FileGood filename -> filename
  | Timeout -> let _ = server_write_msg lastmsg in
                 server_processmsg (attempts + 1) (server_read_msg ())