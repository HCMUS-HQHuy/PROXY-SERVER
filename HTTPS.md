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
OK! Now, let's get it! 
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



# How to setup a HTTPS connection between client and server - ĐỌC THÊM

Khi một trình duyệt web gửi kết nối **HTTPS** đến một server, quá trình giao tiếp giữa trình duyệt và server có một số bước cụ thể. Điều này liên quan đến **SSL/TLS handshake**, quá trình xác thực và mã hóa dữ liệu. Dưới đây là mô tả chi tiết các bước chính khi một kết nối HTTPS được thiết lập:

### 1. **Người dùng nhập URL (bắt đầu kết nối)**
Khi người dùng nhập một URL bắt đầu bằng `https://` vào trình duyệt (ví dụ: `https://example.com`), trình duyệt sẽ thực hiện các bước sau:

### 2. **DNS Lookup (Phân giải tên miền)**
Trình duyệt cần biết địa chỉ IP của server để gửi yêu cầu đến. Do đó, nó sẽ:
- Sử dụng DNS (Domain Name System) để phân giải tên miền (ví dụ: `example.com`) thành địa chỉ IP của server.
- Sau khi nhận được địa chỉ IP của server, trình duyệt sẽ tiếp tục với bước tiếp theo.

### 3. **Tạo kết nối TCP (3-way handshake)**
Để bắt đầu truyền tải dữ liệu, trình duyệt sẽ thiết lập một kết nối TCP đến server:
- Trình duyệt gửi một **SYN** (synchronize) để yêu cầu kết nối.
- Server đáp lại bằng một **SYN-ACK** (synchronize-acknowledge).
- Trình duyệt trả lời bằng **ACK** (acknowledge), tạo thành kết nối TCP giữa client và server.

### 4. **Bắt đầu SSL/TLS Handshake (Khởi tạo kết nối bảo mật)**
Sau khi kết nối TCP được thiết lập, quá trình **SSL/TLS handshake** bắt đầu. Đây là quá trình để thiết lập một kết nối an toàn và mã hóa giữa trình duyệt và server. Các bước của SSL/TLS handshake như sau:

#### a. **Client Hello**
- Trình duyệt (client) gửi một **ClientHello** đến server. Gói tin này bao gồm:
  - **Phiên bản SSL/TLS mà client hỗ trợ** (ví dụ: TLS 1.2, TLS 1.3).
  - **Danh sách các cipher suites** (phương thức mã hóa) mà client hỗ trợ (ví dụ: AES, RSA).
  - **Dữ liệu ngẫu nhiên** (random data) mà client tạo ra.
  - **Số liệu session ID** nếu client muốn tiếp tục một phiên cũ.

#### b. **Server Hello**
- Server nhận gói **ClientHello** và phản hồi bằng một **ServerHello**, trong đó bao gồm:
  - **Phiên bản SSL/TLS được chọn** (theo các tùy chọn của client và server).
  - **Cipher suite được chọn** (một trong những phương thức mã hóa mà client và server đều hỗ trợ).
  - **Dữ liệu ngẫu nhiên** (random data) của server.
  - **Chứng chỉ số (SSL certificate)**: Chứng chỉ này chứa khóa công khai của server và được phát hành bởi một **CA (Certificate Authority)** tin cậy.

#### c. **Chứng thực server (Server Authentication)**
- Server gửi chứng chỉ SSL của mình cho trình duyệt. Chứng chỉ này chứa khóa công khai của server và thông tin xác thực từ CA (chứng chỉ do một cơ quan chứng nhận tin cậy cấp).
- Trình duyệt kiểm tra chứng chỉ này để đảm bảo rằng nó hợp lệ (không hết hạn, thuộc về domain đúng, và được cấp bởi một CA tin cậy).

#### d. **Key Exchange (Trao đổi khóa)**
- Dựa trên các cipher suite và thuật toán được chọn trong handshake, một quy trình trao đổi khóa sẽ diễn ra. Một trong những phương thức phổ biến là **RSA** hoặc **Diffie-Hellman**:
  - Nếu sử dụng **RSA**: Trình duyệt sẽ mã hóa một khóa ngẫu nhiên (symmetric key) bằng khóa công khai của server và gửi khóa này cho server.
  - Nếu sử dụng **Diffie-Hellman**: Cả hai bên sẽ tạo ra các khóa riêng biệt và sau đó tính toán khóa chung mà chỉ client và server có thể biết.

#### e. **Server Finished**
- Sau khi hoàn thành quá trình trao đổi khóa, server sẽ gửi một thông báo **Finished** đã được mã hóa bằng khóa chung, thông báo rằng phần thiết lập bảo mật đã hoàn tất.

#### f. **Client Finished**
- Trình duyệt nhận thông báo từ server và sau đó gửi một thông báo **Finished** của riêng mình, đánh dấu rằng SSL/TLS handshake đã được hoàn thành và dữ liệu có thể được mã hóa.

### 5. **Chuyển dữ liệu mã hóa**
- Sau khi quá trình handshake hoàn tất, tất cả dữ liệu giữa client và server sẽ được mã hóa bằng thuật toán mã hóa **symmetric** (ví dụ: AES).
- Mã hóa đối xứng sử dụng một khóa chung mà cả client và server đã trao đổi trong bước trước.

### 6. **Gửi HTTP Request**
- Sau khi kết nối an toàn đã được thiết lập, trình duyệt sẽ gửi yêu cầu HTTP (ví dụ: GET, POST) đến server.
- Yêu cầu này sẽ được mã hóa trước khi gửi đi. Dữ liệu này có thể bao gồm các thông tin như đường dẫn URL, các headers HTTP, cookies, dữ liệu form, v.v.

### 7. **Server xử lý và trả về HTTP Response**
- Server nhận được yêu cầu HTTP, xử lý và gửi lại một phản hồi HTTP được mã hóa.
- Phản hồi có thể chứa dữ liệu HTML, hình ảnh, JSON hoặc bất kỳ loại dữ liệu nào mà trình duyệt yêu cầu.
- Phản hồi này cũng sẽ được mã hóa và sau đó giải mã bởi trình duyệt.

### 8. **Giải mã và hiển thị dữ liệu**
- Trình duyệt nhận phản hồi đã được mã hóa từ server và giải mã nó bằng khóa chung mà cả hai bên đã trao đổi trong quá trình handshake.
- Sau khi giải mã, trình duyệt sẽ xử lý dữ liệu (ví dụ: hiển thị trang web) và hiển thị nó cho người dùng.

### 9. **Kết thúc phiên (Session Termination)**
- Khi phiên làm việc kết thúc, kết nối giữa client và server sẽ được đóng lại. Trình duyệt có thể gửi một **close_notify** để báo cho server rằng kết nối sẽ được đóng.
- Dữ liệu và khóa mã hóa của phiên sẽ được hủy bỏ, và nếu có bất kỳ yêu cầu tiếp theo nào trong tương lai, một handshake mới sẽ được thực hiện.

### Tổng kết quy trình kết nối HTTPS:
1. **Kết nối TCP** được thiết lập (3-way handshake).
2. **SSL/TLS handshake** để tạo kết nối bảo mật:
   - ClientHello → ServerHello → Chứng thực server.
   - Trao đổi khóa (RSA hoặc Diffie-Hellman).
3. Sau khi SSL/TLS handshake thành công, dữ liệu HTTP sẽ được mã hóa và gửi đi.
4. Server xử lý yêu cầu, gửi phản hồi lại, và trình duyệt giải mã nội dung.
5. Kết thúc phiên.

Thông qua quy trình này, HTTPS bảo đảm rằng:
- **Tính bảo mật**: Dữ liệu giữa client và server được mã hóa và bảo vệ khỏi các cuộc tấn công nghe lén.
- **Tính toàn vẹn**: Dữ liệu không bị thay đổi trong quá trình truyền tải.
- **Xác thực server**: Trình duyệt chắc chắn rằng nó đang giao tiếp với đúng server thông qua chứng chỉ SSL/TLS hợp lệ.


# Some note for flags in g++
+ -l flag là để link các thư viện lại. Nó bỏ extension và bỏ cả prefix lib
+ -I là flag để chỉ định thư mục chứa các file header.
+ -L được sử dụng để chỉ định thư mục chứa các thư viện mà trình biên dịch sẽ tìm kiếm khi linking (liên kết) với các thư viện ngoài trong quá trình biên dịch.