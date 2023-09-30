(* data checksum *)

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
