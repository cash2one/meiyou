nohup python identifymijiserver.py > identify.out 2>&1 &
java -Djava.library.path=/usr/local/share/OpenCV/java/ -jar -Xms1024m -Xmx3072m -XX:+UseParNewGC -XX:+UseConcMarkSweepGC -XX:CMSInitiatingOccupancyFraction=70 -Xloggc:gc.log /data/tclip/meiyou-master/udf.jar