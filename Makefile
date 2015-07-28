all:
	g++ bs.cpp -o bs mcp3008Spi.cpp -l mysqlcppconn -l wiringPi -l mysqlclient
