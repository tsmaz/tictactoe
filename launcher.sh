BROKER="34.102.16.54"
PORT=1883
TOPIC="cs2600/ttt/VMControl"

BASH_PID="./bash.pid"
CAI_PID="./cai.pid"

mosquitto_sub -h $BROKER -p $PORT -t $TOPIC | while read CMD; do
  if [ "$CMD" = "startBashAI" ]; then
    ./tttBot.sh &
    echo $! > $BASH_PID

  elif [ "$CMD" = "stopBashAI" ]; then
    [ -f $BASH_PID ] && kill $(cat $BASH_PID) && rm $BASH_PID

  elif [ "$CMD" = "startCAI" ]; then
    ./auto_player &
    echo $! > $CAI_PID

  elif [ "$CMD" = "stopCAI" ]; then
    [ -f $CAI_PID ] && kill $(cat $CAI_PID) && rm $CAI_PID
  fi
done
