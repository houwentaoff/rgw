This is just for unit test.

build:
    make all

run:

[1] config nginx.

    centos7 : rpm -ivh http://nginx.org/packages/centos/7/x86_64/RPMS/nginx-1.8.0-1.el7.ngx.x86_64.rpm 
    cp -a ../etc/nginx.conf /etc/nginx/

[2] install  spawn-fcgi

[3] copy fcig

    mkdir -p /webSvr/cgi-bin && install -p -v test_fcgi.fcgi -m 0777  /webSvr/cgi-bin/

[4] run : spawn-fcgi -a 127.0.0.1 -p 9003 -f /webSvr/cgi-bin/test_fcgi.fcgi

[5] brower explore http://172.16.133.8:9000/demo.cgi
