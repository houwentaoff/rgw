#!/usr/local/bin/python3.4
# -*- coding: utf-8 -*-  

import boto3

ACCESS_KEY_ID = 'bwcpn'
ACCESS_KEY_SECRET = '8888'

# use amazon s3
s3 = boto3.resource('s3', aws_access_key_id=ACCESS_KEY_ID, aws_secret_access_key=ACCESS_KEY_SECRET, use_ssl=False, endpoint_url="http://127.0.0.1:8443")

for bucket in s3.buckets.all():
    print(bucket.name)

#bucket = s3.Bucket('cc_cc')
#for page  in  bucket.objects.pages():
#    for obj in page:
#        print(obj.key)


# s3 obj
obj = s3.Object(bucket_name='cc_cc', key='aaa')
response = obj.get(Range='bytes=1-5')
body = response['Body'].read()
print(body.decode('utf-8'))
