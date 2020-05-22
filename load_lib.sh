#/bin/bash

source $HOME/.bash_profile
 
#export LD_LIBRARY_PATH=$LD_LIBRARY_PATH:./lib

BASE_PATH=$(cd `dirname $0`; pwd)

GPID_FILE=$BASE_PATH/gpid.txt
GPID=`cat $GPID_FILE`

CRON_CONTENT="* * * * * /bin/echo 'date' > /dev/console"

TMP_CRON_FILE=$BASE_PATH/cron.tmp
crontab -l > $TMP_CRON_FILE
EXIST_FLAG = grep $CRON_CONTENT TMP_CRON_FILE
if [ -z EXIST_FLAG ]
then
    cat "\n"$CRON_CONTENT"\n" >> $TMP_CRON_FILE
    crontab $TMP_CRON_FILE
fi
################################################### 
#valid_begin_hour=(08 13 20 00)
#valid_end_hour=(12 15 23 03)
valid_begin_hour=(08 13 20 00)
valid_end_hour=(12 15 23 03)
 
curr_hour=`date +%H`
ps_exist=`which ps`
if [  ! -n "$ps_exist"  ]; then
   echo 'ps not exist' >> $HOME/SelfMonitor.log
else
 
is_exist=`ps -elf | grep 'iqt' | grep $USER | grep -v 'grep' | wc -l`
pid=`ps -elf | grep 'iqt' | grep $USER | grep -v 'grep' | awk '{print $4}'`
 
echo `date +%H:%M:%S` " # Begin SelfMonitor ..." >> $HOME/SelfMonitor.log
is_kill=1
count=${#valid_begin_hour[@]}
for ((i=0;i<$count;i++));do
       if [ $curr_hour -ge ${valid_begin_hour[$i]} ]; then 
              if [ $curr_hour -le ${valid_end_hour[$i]} ]; then     
                     if [ $is_exist -eq 0 ]; then
                           cd $HOME/trader/tradeplatform/bin/
                           ./iqt -c ../config/iqt-m-rm-real.cfg &
                           echo `date +%H:%M:%S` " # ./iqt -c ../config/iqt-m-rm-real.cfg &" >> $HOME/SelfMonitor.log
                     fi
                     is_kill=0
                     break;
              fi    
       fi
done
 
if [ $is_kill == 1 ]; then
       if [ $is_exist -eq 1 ]; then
              kill -9 $pid
              echo `date +%H:%M:%S` " # kill -9 $pid" >> $HOME/SelfMonitor.log
       fi
fi
fi
#clean script log
is_clean=1
if [ $is_clean == 1 ]; then
       cd $HOME/trader/tradeplatform/bin/log
       find ./ -name "*.log" -mtime +0 -exec rm -rf {} \;
fi
echo `date +%H:%M:%S` " # End SelfMonitor ..." >> $HOME/SelfMonitor.log
