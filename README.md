COMPILE

[1] cd src/json_spirit/ && mkdir -p build && cd build && cmake ../ && make && cp \*.a ../  
[2] cd src/third-party/ && tar -zxvf fcgi.tar.gz && patch -p0 < fcgi.patch (make fcgi)  
[3] cd src/ && make all

INSTALL

[1] yum install boost fcgi nginx

1.1 install nginx
centos6:  
rpm -ivh http://nginx.org/packages/centos/6/noarch/RPMS/nginx-release-centos-6-0.el6.ngx.noarch.rpm  
yum install nginx  
centos7:  
yum install nginx  

[2] config /etc/nginx/nginx.conf(eg: src/etc/nginx2.conf)

RUN

[1] service nginx  start
[2] ./test_main

centos6:  
    cp src/third-party/lib/*   /lib/  
centos7:   
    yum install boost

TEST

[0] config s3cmd config( src/etc/.s3cfg). use s3cmd. 
