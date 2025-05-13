BROKER="localhost"
BOARD="cs2600/ttt/board"
OUT="cs2600/ttt/clientToServer"

while true; do
  BOARD_STATE=$(mosquitto_sub -h $BROKER -t $BOARD -C 1)

  XCOUNT=$(echo "$BOARD_STATE" | grep -o X | wc -l)
  OCOUNT=$(echo "$BOARD_STATE" | grep -o O | wc -l)

  if [ $XCOUNT -eq $((OCOUNT + 1)) ]; then
    empties=()
    for POS in $(seq 0 8); do
      if [ "${BOARD_STATE:$POS:1}" = "E" ]; then
        empties+=($POS)
      fi
    done

    LEN=${#empties[@]}
    if [ $LEN -gt 0 ]; then
      IDX=$(( RANDOM % LEN ))
      POS=${empties[$IDX]}

      ROW=$(( POS / 3 ))
      COL=$(( POS % 3 ))

      if [ $COL -eq 0 ]; then FILE="A"; fi
      if [ $COL -eq 1 ]; then FILE="B"; fi
      if [ $COL -eq 2 ]; then FILE="C"; fi
      RANK=$(( ROW + 1 ))

      mosquitto_pub -h $BROKER -t $OUT -m "play $FILE$RANK"
    fi
  fi
done
