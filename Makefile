all:
	g++ bs.cpp -o bs -l mysqlcppconn -l wiringPi -l mysqlclient
