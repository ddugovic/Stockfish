#!/bin/bash
# evaluate performance in test positions

error()
{
  echo "puzzle testing failed on line $1"
  exit 1
}
trap 'error ${LINENO}' ERR

echo "puzzle testing started"

expect << EOF
   set timeout 600
   spawn python3 ../tests/chess-artist/chess_artist.py --infile ../tests/chess-artist/EPD/wacnew.epd --outfile wacnew_result.txt --enginefile ./stockfish --eval static --job test
   expect -re "Correct percentage    : (100|9\\\\d)"
   expect eof
EOF

rm wacnew_result.txt

echo "puzzle testing OK"
