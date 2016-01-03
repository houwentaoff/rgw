COMPILE

[1] cd src/json_spirit/ && mkdir -p build && cd build && cmake ../ && make && cp \*.a ../
[2] cd src/ && make all

INSTALL

[1] yum install boost fcgi nginx
[2] config /etc/nginx/nginx.conf

RUN

[1] service nginx  start
[2] ./test_main

TEST

[1] config s3cmd config. use s3cmd. 
