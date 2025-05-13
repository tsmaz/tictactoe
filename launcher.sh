BROKER="localhost"
TOPIC="cs2600/ttt/vmControl"
PIDFILE="./ai.pid"

mosquitto_sub -h $BROKER -t $TOPIC | while read CMD
do
  if [ "$CMD" = "startAI" ]; then
    echo "Launcher: starting AI"
    ./ai_agent.sh &
    echo $! > $PIDFILE
  fi

  if [ "$CMD" = "stopAI" ]; then
    echo "Launcher: stopping AI"
    if [ -f $PIDFILE ]; then
      kill $(cat $PIDFILE)
      rm $PIDFILE
    fi
  fi
done
