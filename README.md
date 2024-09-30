overall   vedio <https://www.youtube.com/watch?v=lvjxdnW8Dd4>  
demo/spec vedio by TA <https://www.youtube.com/watch?v=TdXyYWX_HW8>

# Systems programming 2024: csieTrain

## 1. Problem Description

In a bustling city where trains are essential for daily commuters, the railway authorities sought to improve the booking experience by introducing a new system called **`csieTrain`**. This system was designed to simplify **`seat reservations`**, **`cancellations`**, and **`payments`**, making it easier for passengers to book their seats efficiently.

Thousands of passengers attempt to book seats on their preferred train shifts every day. The old system, running on an overloaded workstation, often caused slow responses and double bookings, frustrating many users. To address the issue, as a skilled student, you have been tasked with building a robust csieTrain system capable of handling multiple requests simultaneously while managing bookings securely. Your system must efficiently process client requests, even under heavy loads, ensuring a smooth experience for all passengers.

<!-- Every day, thousands of passengers try to book seats on their preferred train shifts. The old server, running on an overloaded workstation, often resulted in slow responses and frequent double bookings, leading to many complaints from frustrated passengers. To address this, as a skilled CSIE student, you have been tasked with building a robust **csieTrain** system that can handle multiple requests simultaneously and manage bookings securely. Your system should be efficiently process client requests, even under heavy loads, ensuring a smooth experience for all passengers. -->

### 1.1 csieTrain Overview

The csieTrain system consists of a **`read server`** and a **`write server`**. Both servers can access the **`csie_trains`** directory, which contains records representing the seat maps of different train shifts. The read server allows passengers to check seat availability, while the write server manages reservations and updates seat records.

Some passengers would connect to the servers and either take too long browsing seat availability or reserve seats without completing payment, causing outdated information or blocking others from booking. Additionally, passengers could also face connection issues, leading to incomplete requests that might hang the system. Your csieTrain system should also handle such issues efficiently, ensuring smooth and fair access for all passengers.

### 1.2 Requirements Overview
You are expected to complete the following tasks:

1. **Implement the servers**: The provided sample source code `server.c` can be compiled as simple read/write servers, but feel free to edit any part of codes.
2. **Handle Multiple Requests Simultanenously**: Modify the servers in order that the servers will not be blocked by any single request, but can deal with many requests simultaneously. (Hint: implement multiplexing with `select()` or `poll()` and refrain from busy waiting) 
3. **Record Consistency**: Guarantee the correctness of seat records when it is being modified. ( Hint: use file lock to prevent simultaneous modifications ) 
4. **Handle Connection Timeout**: Automatically close connections after a set period. ( Hint: implement a timeout mechanism, freeing up resources and preventing blocked reservations. )
5. **Handle Connection Issues:** Ensure the servers can handle connection disruptions, such as slow or dropped connections, without impacting other client requests. (Hint: handle incomplete messages)

<!-- Some passengers would connect to the servers and either spend too much time viewing seat availability or reserve seats without paying. This led to passengers holding outdated information or blocking others with unpaid reservations. To prevent this, the csieTrain system was enhanced with a timeout mechanism. If a client stays connected for too long, the server automatically closes the connection and releases any reserved seats. -->

<!-- the read server will ask the client which shift's seat map they want to check. The write server will ask the client which shift they want to book a seat on and can modify the seat map files to update the seating information accordingly.  -->

## 2. Run the sample Servers

The provided sample code can be compiled as sample read/write servers. The sample servers has handled the part of socket programming, so you are able to connect to the sample servers once you run it.

Feel free to modify any part of the code as you need, or implement your own server from scratch.
### Compile
You should write your own `Makefile` to compile your code. You can use the provided command to compile sample source code:
```bash
$ gcc server.c -D WRITE_SERVER -o write_server
$ gcc server.c -D READ_SERVER -o read_server
```
Your `Makefile` may contain commands above to generate `write_server` and `read_server`.
Also, your `Makefile` should be able to perform cleanup after the execution correctly (i.e, delete `read_server` and `write_server`).
```Makefile
clean:
    ## TODO：
    ## delete write_server and read_server
```
### Run
After you compile the code, you can run the server with following command:
```bash
$ ./write_server {port}
$ ./read_server {port}
```
For example, `./write_server 7777` runs a write server listening on port 7777, and `./read_server 8888` runs a read server listening on port 8888.
Note that port 0~1023 are reserved for common TCP/IP applications, so you should not pick a number within this range.
## 3. Test your Servers at Client side
You can either write a script or use the following command to connect to a running server:
```bash
$ telnet $hostname $port
```
For example, if you run `./read_server 7777` on CSIE ws1 workstation, you can interact with this read server with `telnet ws1.csie.ntu.edu.tw 7777`. 

Similarly, if you run `./write_server 10000` on localhost, you can interact with this write server with `telnet 127.0.0.1 10000`.

TAs will test your code on `ws1`. Make sure your code works as you expect on it.

## 4. About the record files
The servers will access the files `train_902001` to `train_902005`, which represent the seat maps of different train shifts, under the `csie_trains` directory. 
```
$ ls ./csie_trains
train_902001  train_902002  train_902003  train_902004  train_902005
```
Each file contains 40 seats available for selection, and each seat has two possible seat states ( represented by two integers 0,1 ), which are explained as follows:
* `0` represents a seat that is available for selection.
* `1` represents a seat that has already been booked and cannot be selected.
<!-- * `2` represents a seat that has been selected by a process (i.e., write server) but not yet  booked, and cannot be selected by others. -->

<!-- Following is the structure of a record defined in `server.h` : 
```c
#define TRAIN_NUM 5
#define SEAT_NUM 40
#define TRAIN_ID_START 902001
#define TRAIN_ID_END TRAIN_ID_START + (TRAIN_NUM - 1)
typedef struct {
    int id;
    int train_fd;
    int book_num;
    int chosen_seat[SEAT_NUM];
} record;
``` 
Note that when judging your homework, TAs may use **another** header file during the test. -->

## 5. Sample input and output
### 5.0 Banner
:::spoiler {state='open'} **Click me**
Upon connecting to the server, the terminal displays the following banner:
```
======================================
 Welcome to CSIE Train Booking System 
======================================
```
:::

### 5.1 Read Server
The `read_server` allows clients to check seat availability for each train shift. 
:::spoiler {state='open'} **Click me**
    
1. <a id="r1"> The client should see the message:
    `Please select the shift you want to check [902001-902005]: `</a>

2. If the client enters a valid train shift ID (e.g., `902001`), the server should respond with the seat map for that shift. For example:
    `Please select the shift you want to check [902001-902005]: 902001`
    Then, 
    ```
    1 1 0 0
    1 0 1 1
    1 1 0 1
    0 1 0 1
    1 1 0 1
    0 0 0 0
    0 0 1 0
    0 2 0 0
    1 2 2 0
    0 0 0 1
    ```
    In this map:
    - `0` indicates an available seat.
    - `1` indicates a booked seat.
    - `2` indicates a seat reserved by others

    :bulb: Note that only `0` and `1` are recorded in the file. Please do not write `2` in your record files.

4. After printing the seat map, return to [1.](#r1) handle further check requests
:::

### 5.2 Write Server
The `write_server` allows clients to select a train shift, reserve seats, and process payments. 
:::spoiler {state='open'} **Click me**
#### Select a Shift
1. <a id="w1">The client should see the message:
    `Please select the shift you want to book [902001-902005]: `</a>

2. If the client enters a valid train shift ID (e.g., `902001`), the server should respond:
    - If the shift is fully booked (i.e., no available seats, no reservations). Display following messaage and return to [1.](#w1) to handle further book requests.
        `>>> The shift is fully booked.`   
    - Otherwise, the client should see the message:
    ```
    Booking info
    |- Shift ID: 902001
    |- Chose seat(s):
    |- Paid:
    ```
    Here, `Shift ID: 902001` reflects the selected train shift.

<!-- If the client types a valid train shift ID (for example, 902001), the server will reply with their booking infomation, following by a prompt: -->

#### Select a Seat
3. Then, the client should see the message:
    `Select the seat [1-40] or type "pay" to confirm: `
4. The client can enters a valid seat ID (e.g., `34`), the server should respond:
    - If the client **selects a seat that is available**, the server successfully reserves that seat and adds the selected seat ID to the `Chose seat(s)` section of the `Booking info`:
    ```
    Booking info
    |- Shift ID: 902001
    |- Chose seat(s): 34
    |- Paid:
    ```

    
    - If the client **selects a seat that has already been booked (i.e., paid)**, the server will respond with the following message:
    ```
    >>> The seat is booked.

    Booking info
    |- Shift ID: 902001
    |- Chose seat(s):
    |- Paid:
    ```
    - If the client **selects a seat that is being reserved by others**, the server will respond with the following message:
    ```
    >>> Locked.

    Booking info
    |- Shift ID: 902001
    |- Chose seat(s):
    |- Paid:
    ```
5. Everytime after the client selected a seat, the client should see the message:
`Select the seat [1-40] or type "pay" to confirm: `
Then, the client can then choose to continue [**selecting seats**](#Select-Multiple-Seats) or [**cancel the selected seat**](#Cancel-the-Selected-Seat) or [**proceed to payment**](#Proceed-to-Payment). 

#### Select Multiple Seats
6. If the client has already chosen some seats, the IDs in the `Chose seat(s)` section should be displayed in numerical ascending order, separated by commas`,`. For example, if the client reserves seats `34`, then `30`, and then `35`, the message will be:
    ```
    Booking info
    |- Shift ID: 902001
    |- Chose seat(s): 30,34,35
    |- Paid:    
    ```
#### Cancel the Selected Seat
7. The client can cancel a selected seat by typing the seat ID again. For example, if the client has reserved seats `30`,`34`,`35`:
    ```
    ...
    Booking info
    |- Shift ID: 902001
    |- Chose seat(s): 30,34,35
    |- Paid: 
    
    Select the seat [1-40] or type "pay" to confirm: 34
    >>> You cancel the seat.

    Booking info
    |- Shift ID: 902001
    |- Chose seat(s): 30,35
    |- Paid: 

    Select the seat [1-40] or type "pay" to confirm:
    ```
#### Proceed to Payment

8. If the client **choose to pay**, all selected seats will be processed for payment. The server will respond with the following message, if:
    - The client chooses to pay without having selected any seats:
    ```
    Select the seat [1-40] or type "pay" to confirm: pay
    >>> No seat to pay.

    Booking info
    |- Shift ID: 902001
    |- Chose seat(s):
    |- Paid: 1,2

    Select the seat [1-40] or type "pay" to confirm:
    ```
    - Otherwise, moving the selected seats (for example, seat IDs `30` and `35`) to the `Paid` section of the booking information. The paid seats should be displayed in numerical ascending order, separated by commas`,`: 
    ```
    Select the seat [1-40] or type "pay" to confirm: pay
    >>> Your train booking is successful.

    Booking info
    |- Shift ID: 902001
    |- Chose seat(s):
    |- Paid: 30,35

    Type "seat" to continue or "exit" to quit [seat/exit]:
    ```
    
#### After Payment
9. Once the payment is successful, the client can enter `seat` to continue [selecting additional seats](#Select-a-Seat). If the client chooses `exit`, the server will reply with the following message and close the connection with the client.
    `>>> Client exit.`

:::
### 5.3 Handling Client Connections
#### 5.3.1 Handle Connection Timeout
Both `read_server` and `write_server` allow each connection to remain open for **5 seconds**. After this period, the server will automatically close the connection. Ensure that your implementation does not cause busy waiting.

:::info
:bulb: Hint: You may use `clock()` to track the execution time of each request. While how can you compute the connection time if the requests are idle?
:::

#### 5.3.2 Handle Connection Issues
Clients may experience connection issues, such as sending messages slowly, resulting in incomplete messages received by the server. However, the server should not become stuck waiting for a message and must be able to handle other requests.


:::info
:bulb: Hint: You may consider modifying the `handle_read()` function
:::

### 5.4 Input Checking
:::spoiler {state='open'} **Click me**
#### Input format
- Each input ending with a newline `\n` is treated as a **single valid input**.
- The length of a **single valid input** must be less than `MAX_MSG_LEN` (512 bytes)
- A **single valid input** is treated as a **command**.
- If an invalid command is entered, it is considered a failed operation, and the server will respond:
`>>> Invalid operation.`
The server will then close the connection from the client.
- The command used in wrong state (Shift Selection, Reservation, Payment) is considered as invalid command. For example, 
    - Entering `pay` or `seat` while choosing a shift.
    - Entering `seat` during seat selection. 
    - Entering `pay` command, a valid shift ID, or a seat ID after completing a payment (when prompted to enter `seat` or `exit`). 

    The server will respond:
    `>>> Invalid operation.`
    Then, close the connection from the client.
    
#### Exit the server
Regardless of whether connected to the read or write server, if **`exit`** is entered at any time, the connection to the server will be terminated, and the following message will be displayed in the terminal:
`>>> Client exit.`

#### Note
All server responses will be displayed on the client. Although server output isn’t directly checked, proper error handling and resource cleanup are essential for good programming. Avoid printing unnecessary or debug messages to maintain a clear session on the server.
:::


## 6. Report and Grading
:::danger
**Warning**
* Please strictly follow the output format above, or you will lose the point of each task, respectively.
* Make sure your `Makefile` compile files correctly on ws1, or your score will be **0**.
* Make sure your `Makefile` can clean unnecessary files with `make clean`, or you will lose **0.25 point**.
* You should not use `fork()` or threading in this assignment, or your score will be **0**.
:::
### 6.1 Report 
1. What is busy waiting? How do you avoid busy waiting in this assignment? Is it possible to have busy waiting even with `select()`/`poll()` ?
2. What is starvation? Is it possible for a request to encounter starvation in this assignment? Please explain.
3. How do you handle a file's consistency when multiple requests within a process access to it simultaneously?
4. How do you handle a file's consistency when different process access to it simultaneously?

### 6.2 Grading
<!-- 
2 public testcases
0.5 basic

 -->
1. Basic operations **(Public: 0.5 point)**
    - Valid commands (response from server will be ignored):
        - Shift ID [902001-902005]
        - Seat ID [1-40]
        - Commands: `pay`, `seat`
    - Exit and Invalid operations (The respones listed in [Input Checking](#54-Input-Checking))
2. Handle requests on 1 server, while the server will only have 1 connection simultaneously. 
    - 1 read server **(Public: 0.2 point)** 
    - 1 write server **(Public: 0.5 point, Private: 0.5 point)**  
3. Handle requests on 1 read server and 1 write server, and each server will have 1 conection simultaneously. **(Public: 0.4 point, Private: 0.8 point)** 
4. Handle requests on 1 server, while there will be multiple connections simultaneously. **(Public: 0.4 points, Private: 0.8 point)** 
5. Handle requests on multiple servers (at most 4 servers), while there will be multiple connections simultaneously and different servers may access `csie_trains` simultaneously. 
    * Clients will always finish their operations **(Private: 1.2 point)**
    * Clients may close their connections at any stage of their operations **(Private: 0.5 point)**
6. Handle various connection scenarios across multiple servers (at most 4 servers) with multiple connections simultaneous. **You will not get following 1.2 point if your server causes busy waiting**:
    - Clients may experience connection issues. **(Private: 0.4 point)** 
    - Clients may exceed the 5-second timeout for requests. **(Private: 0.8 point)**
    

8. Report **(1 point)**
    * Please answer the [problems](#61-Report) and submit on Homework section of NTU COOL
<!--6. Handle requests on 1 read server and 1 write server, and there will be 1 client for read server and 2 clients for write server. (1.2 point)-->
<!--
### Test cases
- read server (0.2%)
    - check seat info (0.2%)
- write server (1%)
    - the seat is booked (0.1%)
    - chose multiple seats (0.1%)
    - pay (0.1%)
    - After paid, choose the paid seats (0.1%)
    - After paid, choose the empty seats (0.1%)
    - Cancel the seats (0.1%)
    - null pay (case 1: haven't chosen seats, case 2: paid seat but haven't chosen addition seats) (0.2%)
    - TODO： the shift is fully booked. (0.2%)

- read server & write server (1 client for each) (1.2%)
    - chose seat, read server should see "2" (0.4%)
    - after paid, read server should see "1" (0.4%)
    - cancel seat, read server should see "0" (0.4%)

- 1 write server (multiple connections) (1.2%)
    - A chose and B get locked. (0.4%)
    - A canceled and B chose (0.4%)
    - B paid and A check the seat is booked (0.4%)

- multiple write servers (x client for each) （1.2%）
    - A chose and B get locked. （0.4 %）
    - A canceled and B chose （0.4%）
    - B paid and A check the seat is booked （0.4%）

- 1 read server 1 write server (1 for read, 2 for write) (1.2%)
    - A chose seat x, B chose seat x and get locked 
    - C check seat info of x in read server（1.2%）


- client may disconnect arbitrarily (0.5%)
    - read server & write server
    - chose restoration and paid (0.3/0.2) 
-->

## 7. Submission
<!-- > 寫清楚會只看最後一個commit，請注意上傳時間
> 0分之後的時間，要讓他不能summit -->

### To GitHub
:::spoiler {state="open"} **Click me**
The folder structure on github classroom should be
```
.
├── csie_trains
│   ├── train_902001
│   ├── train_902002
│   ├── train_902003
│   ├── train_902004
│   └── train_902005
├── Makefile
├── server.c
└── server.h

```
Your source code should be submitted to GitHub before deadline. You can submit other `.c`, `.h` files, as long as they can be compiled to two executable files named `read_server` and `write_server` with `Makefile`. **TA will clone your repository and test your code with last commit before deadline on `main` branch, which is the default branch of GitHub**. You are encouraged to submit a `README.md`, where you can briefly state how do you finish your program and something valuable you want to explain.

Furthermore, TAs will test your code on **`ws1.csie.ntu.edu.tw`**, so make sure your code will work as expected on this server. If you print some debugging messages in server, it's suggested that you comment these debugging messages out, otherwise it may slow down your server and affect its ability to handle requests within the timeout mechanism.

Last but not least,
- **do not** submit files generated by `Makefile`. You should `make clean` before you submit. <font color="red">You will lose 1 point if you submit unnecessary files.</font>
:::
### To NTU COOL
:::spoiler {state="open"} **Click me**
Your **report** should be submitted to NTU COOL before deadline.
:::
## 8. Reminder
:::spoiler {state="open"} **Click me**

1. **Plagiarism is STRICTLY prohibited.**
    - Both copying and being copied result in a score of zero.
    - Previous or classmates' work will be considered as plagiarism.
    - Discussion is allowed, but **the code should be entirely self-written**.
    - Avoid letting others view your code. It's your responsibility to protect your work.
    - The method of checking goes beyond comparing source code similarity (e.g., changing `for` to `while` or modifying variable names will also be detected).
2.  Late policy (D refers to formal deadline, **10/11 23:59**)
    * If you submit your assignment **on D+1 or D+2**, your score will be multiplied by **0.85**.
    * If you submit your assignment **between D+3 and D+5**, your score will be multiplied by **0.7**.
    * If you submit your assignment **between D+6 and 10/18**, your score will be multiplied by **0.5**.
    * Late submission after **10/18 23:59** will not be accepted.
3. If you have any question, feel free to:
    - evoke issues in [SP2024_HW1_release](https://github.com/NTU-SP/SP2024_HW1_release)
    - contact the TAs via email: 
        - class 1: [ntusp2024.pj@gmail.com](mailto:ntusp2024.pj@gmail.com)
        - class 2: [ntusp.class2@csie.ntu.edu.tw](mailto:ntusp.class2@csie.ntu.edu.tw)
    - ask during TA hours.
4. Please start your work as soon as possible, do **NOT** leave it until the last day!
5. It’s suggested that you should check return value of system calls like open, select, etc.
6. Reading manuals of system calls may be tedious, but it’s definitely helpful.
:::

## FAQ


1. Why is the penalty for not running `make clean` different in Sections 6 and 7 (0.25 and 1 point, respectively)?
**Ans:** The penalty should be 0.25 points. Sorry for confused. Also, if you have a delay penalty, the final score will be calculated as:
     - $max(0, testgrade - 0.25) \times delay\_penalty$
     
2. Can I add `.gitignore` and README.md to my GitHub repo?
**Ans:** Yes, you can include both in your repository.

3. What if I accidentally modified or deleted records in `./csie_trains/` on my repository?
**Ans:** These records are provided for your testing purposes. Feel free to modify them. The private test cases will use their own records, and the format will follow the one in the release.

4. The specification states that the server will close the connection after 5 seconds, but in the demo video, the client connections appear to stay open. Why?
**Ans:** For demo purposes, TA modified the demo implementation. Please follow the homework specifications for your implementation.

5. What is the timeout mechanism for the server? Does it close the connection if the client is idle for more than 5 seconds, or does it close after 5 seconds regardless?
**Ans:** The connection closes 5 seconds after it is established, regardless of activity. 
**A timing deviation of 0.1 seconds is allowed, meaning the connection should close between 4.9 and 5.1 seconds.**
    - Example:
          - Client A connects at time 0 and stays idle or sends requests. It will be closed at time 5.
          - Client B connects at time 1 and will be closed at time 6
    ```
    Client A: accept_conn() -> request ...   -> closed by server 
    Client B:               -> accept_conn() -> idle ...    -> request ...      -> closed by server  
      Time         0                1              5                                      6 
    ```
6. Should the message `>>> The shift is fully booked.` be displayed if clients continue to choose seats after one have completed a payment and the shift becomes full?
**Ans:** No, the message `>>> The shift is fully booked.` should only be displayed when a client is selecting a shift and it is fully booked at that moment.