# Basic knowledgement in cryptography 

## Key and certificates
They are two basic keywords in cryptography and essential for SSL/TLS protocols to protect data in network.
### Keys
+ This is a bit sequence. An encryption algorithm requires a key to encript and decript data.
+ The are 3 main keys in OpenSSL:
    * Public keys: they are used to encript and verify.
    * Private Key: private keys are used to decript and create digital signature.
    * Symmetric Key: both have a same key.  
### Certificates
+ Digital certificate is a documentary used to verify a person. They include public key and identify in formation. Furthermore, they are signed by an organization called **Certification Authority (CA)**.
#### Include:
+ Public key
+ Identify information
+ CA signature
+ Due date

# Main idea:

There are two huge problems:
+ Firstly, how to read HTTPS request from client (browser)
+ Secondly, how to send request to server (real server) through out HTTPS protocols.

We will use OPENSSL to solve these problems.
OK! Now, let get it! 
## First huge problem:
**Solution:** We will make browser trust our own proxy server as a real server so that it can send to us through SSL and we get HTTP request (decripted request)
## Second huse problme:
**Solution:** We will forward request (decripted request) to real server and get HTTP response. 
But, we will have to format the request from brwoser before we send. Because the configuration is not suitable.

# How to setup a certification for my proxy server (self-signed certificate)

There are some main steps:

## Create private key:

This is PRIVATE KEY so DO NOT SHARE IT! 

    openssl genrsa -out root.key 2048

This command creates a private key RSA and protect it by password (AES256)

## Create Self-Signed Certificate:
After having a private key, create a certification:
    openssl req -x509 -new -nodes -key root.key -sha256 -days 365 -out root.crt
They will require you some information:
* Country Name (2 letter code) [AU]:VN
* State or Province Name (full name) [Some-State]:HCM
* Locality Name (eg, city) []:HCM
* Organization Name (eg, company) [Internet Widgits Pty Ltd]:MyTestOrg
* Organizational Unit Name (eg, section) []:IT Department
* Common Name (e.g. server FQDN or YOUR name) []:localhost
* Email Address []:.


## Config file: 
**wikipedia_openssl.cnf**

    [req]
    default_bits       = 2048
    prompt             = no
    default_md         = sha256
    distinguished_name = dn
    req_extensions     = req_ext

    [dn]
    CN = www.wikipedia.org

    [req_ext]
    subjectAltName = @alt_names

    [alt_names]
    DNS.1 = www.wikipedia.org
    DNS.2 = wikipedia.org

## Create cerrtificate
    openssl genpkey -algorithm RSA -out wikipedia.key -pkeyopt rsa_keygen_bits:2048
    openssl req -new -key wikipedia.key -out wikipedia.csr -config wikipedia_openssl.cnf
    openssl x509 -req -in wikipedia.csr -CA root.crt -CAkey root.key -CAcreateserial -out wikipedia.crt -days 365 -sha256 -extfile wikipedia_openssl.cnf -extensions req_ext

### RUN COMMAND
    g++ .\DEMO_READING_HTTPS.cpp -lws2_32 -I"C:/Program Files/OpenSSL-Win64/include" -L"C:\Program Files\OpenSSL-Win64\lib\VC\x64\MT" -lssl -lcrypto -fpermissive

# SOME NOTES:
1. Serial number
2. SAN for certificates
3. Modify request 

# Some note for flags in g++
+ -l flag là để link các thư viện lại. Nó bỏ extension và bỏ cả prefix lib
+ -I là flag để chỉ định thư mục chứa các file header.
+ -L được sử dụng để chỉ định thư mục chứa các thư viện mà trình biên dịch sẽ tìm kiếm khi linking (liên kết) với các thư viện ngoài trong quá trình biên dịch.