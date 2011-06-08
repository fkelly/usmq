#SERVER_PORT -*- coding: utf-8 -*-
# TCP client example

# TODO: need to make sure that the queue is empty, or we need to empty the queue

import socket
import sys
import random
import time

HOST='localhost'
PORT=8788
EMPTY_MESSAGE='\x00'

def set_diff_sample(set1,set2):
    """ Given set1 and set2 that differ, returns an example of 1 element from set1
        not in set2 (if it exists) and likewise for set2 """
    try: 
        not_in_set2=list(set1 - set2)[0]
    except: not_in_set2=None
    try: 
        not_in_set1=list(set2 - set1)[0]
    except: not_in_set1=None
    return not_in_set1, not_in_set2


class Message(object):

    def __init__(self, head, body):
        self.head=head
        self.body=body

    def is_empty(self):
        return self.head is None and self.body is None

    def __eq__(self,m):
        return self.head==m.head and self.body==m.body

    def __len__(self):
        return len(self.head) + len(self.body)

    def __repr__(self):
        if self.is_empty(): return ""
        return self.head + "\r\n" + self.body

def debugprint(dbg, s):
    if dbg: print s 

socket_calls=0
def createSocket(host,port):
    global socket_calls
    socket_calls += 1
    #print "Creating socket (called: %d)" % socket_calls
    client_socket = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
    client_socket.connect((host,port))
    return client_socket

def generateRandomJapaneseString(minlength, maxlength, minlines, maxlines):
    msg=[]
    base="""東日本大震災に関する情報 ( 消息情報、避難所、義援金受付など ）をまとめたサイトを開設しています。"""
    base=unicode(base,'utf-8')
    num_lines=random.randint(minlines,maxlines)
    for i in range(0,num_lines):
        num_chars=random.randint(minlength,maxlength)
        buffer=[]
        for j in range(0,len(base)):
            c=base[random.randint(0,len(base)-1)]
            buffer.append(c)
        s=unicode('').join(buffer)
        msg.append(s)
    msg=unicode('\r\n').join(msg)
    return msg.encode('utf-8')

def generateRandomString(minlength, maxlength, minlines, maxlines):
    msg=[]
    num_lines=random.randint(minlines,maxlines)
    for i in range(0,num_lines):
        num_chars=random.randint(minlength,maxlength)
        buffer=[]
        for j in range(0,num_chars):
            c=chr(97 + random.randint(0,25))
            buffer.append(c)
        s=''.join(buffer)
        msg.append(s)
    return '\r\n'.join(msg)

class client(object):

    def __init__(self,host,port,bytes_to_read=4096, debug=0):
        self.host=host
        self.port=port
        self.socket=createSocket(self.host,self.port)
        self.n=bytes_to_read
        self.debug=debug

    def close(self):
        self.socket.close()

    def reopen(self):
        try: self.close()
        except: pass
        self.socket=createSocket(self.host,self.port)

    def createMessage(self,head,body):
        """
        put messages have the following format:
            PUT
            \r\n
            <head>
            \r\n
            <nbytes>
            \r\n
            <body>
            \r\n
        """
        nbytes=len(body)
        msg='PUT\r\n%s\r\n%d\r\n%s\r\n' % (head,nbytes,body)
        return msg
    
    def put(self, head, msg):
        msg=self.createMessage(head, msg)
        self.socket.send(msg)
        debugprint(self.debug,"Sending message (%s): %s" % (len(msg),msg))
        recv=self.socket.recv(self.n)
        return recv
    
    def count(self):
        header="COUNT\r\n"
        self.socket.send(header)
        recv=self.socket.recv(self.n)
        return int(recv)
    
    def get(self,extra=""):
        """ send a GET request
            extra is an additional string that can be tacked
            on for testing purposes
        """
        header="GET\r\n%s" % extra
        if (extra): print "Sending %d extra bytes." % len(extra)
        self.socket.send(header)
        recv=self.socket.recv(self.n)
        return self._parse_received(recv)

    def match(self,head):
        header="MATCH\r\n%s\r\n" % head
        self.socket.send(header)
        recv=self.socket.recv(self.n)
        return self._parse_received(recv)

    def _parse_received(self,recv):
        # last two chars should be \r\n
        # and this is probably not the best way to do this...
        if recv==EMPTY_MESSAGE: return Message(None, None)
        if len(recv)>=2:
            if recv[-1]=='\n': recv=recv[0:len(recv)-1]
            if recv[-1]=='\r': recv=recv[0:len(recv)-1]
        lines=recv.split('\r\n')
        head=lines[0]
        body='\r\n'.join(lines[1:])
        return Message(head,body)
    
    def send_random_valid(minlength, maxlength, minlines, maxlines):    
        msg=generateRandomString(minlength, maxlength, minlines, maxlines)
        head=generateRandomString(minlength, maxlength, 1, 1)
        return self.put(head,msg)
    
    def send(self,msg):
        """ unlike put, doesn't append PUT to the message --
            good for sending invalid messages"""
        self.socket.send(msg)
        debugprint(self.debug,"Sending message (%s): %s" % (len(msg),msg))
        recv=self.socket.recv(self.n)
        debugprint(self.debug,"Received: %s" % (recv))
        return recv

    def empty(self):
        while self.count(): self.get()

# TODO: use python's unit-testing framework
class test_mq(object):

    def __init__(self, minlength=100, maxlength=300, minlines=1, maxlines=10,debug=0):
        self.minlength=minlength
        self.maxlength=maxlength
        self.minlines=minlines
        self.maxlines=maxlines
        self.debug=debug

    def empty_queue(self,c):
        c.empty()
        count=c.count()
        assert count==0

    def test_get_extra(self,N):
        print "Running: test_get_extra, N=%s" % N
        # send N messages whose length is 0
        c=client(HOST,PORT,debug=self.debug)
        self.empty_queue(c)
        messages=[]
        return_messages=[]
        lenN=len("%d"% N)
        for i in range(0,N):
            li=len("%d" % i)
            num= "%s%d" % ("0"*(lenN-li),i) # pad with 0s
            head="head-%s" % num
            body="body-%s" % num 
            c.put(head,body)
        msglen=len("head-") + len("body-") + 2 * lenN
        import pdb ; pdb.set_trace()
        for i in range(0,N):
            extra_bytes="a"*(4096 + random.randint(1,100))
            retmsg=c.get(extra=extra_bytes)
            retlen=len(retmsg)
            assert retlen==msglen, "We expected the returned message to be length %d, but it was %d: %s." % \
                      (msglen, retlen, retmsg)
        new_count=c.count()
        assert new_count==0
        print "Passed: test_get_extra, N=%s" % N

    def test_empty_messages(self,N):
        print "Running: test_empty_messages, N=%s" % N
        # send N messages whose length is 0
        c=client(HOST,PORT,debug=self.debug)
        self.empty_queue(c)
        messages=[]
        return_messages=[]
        for i in range(0,N):
            msg=""
            messages.append(msg)
        for i in range(0,N):
            c.put("",messages[i])
        for i in range(0,N):
            assert len(c.get())==0
        new_count=c.count()
        assert new_count==0
        print "Passed: test_empty_messages, N=%s" % N

    def test_long_messages(self,N):
        # send N messages whose length > 4096
        # the result should be that the messages are stored, but truncated
        c=client(HOST,PORT,debug=self.debug)
        count=c.count()
        messages=[]
        return_messages=[]
        for i in range(0,N):
            msg="a"*(4096 + random.randint(1,100))
            head="head-%d" % random.randint(10,99)
            messages.append(msg)
        for i in range(0,N):
            c.put(head,messages[i])
        # confirm that message length is 4089
        # 4089 = 4096 - len("PUT\r\n") - len("\r\n") - the trailing CRLF + len(head)A
        # and len(head)=7
        for i in range(0,N):
            rmsg=c.get()
            assert len(rmsg)==4089, "Message is supposed to have length 4096, but it has length %d." % len(rmsg)
        new_count=c.count()
        assert new_count==count
            
    def test_invalid_messages(self,N):
        print "Running: test_invalid_messages, N=%s" % N
        c=client(HOST,PORT,debug=self.debug)
        self.empty_queue(c)
        count=c.count()
        assert count==0, "Message queue should be empty."
        invalid_messages=[]
        return_messages=[]
        for i in range(0,N):
            # create a string without the PUT or other elements of proper msg structure
            illformed_msg="a"*(4096 + random.randint(1,100))
            invalid_messages.append(illformed_msg)
        counter=0
        while counter<N: 
            validmsg="%d this is a valid message" % counter 
            # send some random number of bad messages
            num_send=random.randint(1,N-counter)
            for j in range(0,num_send): c.send(invalid_messages[j])
            counter += num_send
            # validate that there are no valid messages in the queue 
            assert c.get().is_empty()
            assert c.count()==0
            # put a valid message in the queue
            c.put("head",validmsg)
            msgback=c.get()
            # check that it's there
            assert msgback==Message('head',validmsg), "We expected '%s' but got '%s'" % (validmsg,msgback)
        new_count=c.count()
        assert new_count==count, "New count (%d) should be equal to count (%d)." % (new_count,count)
        for i in range(0,N):
            assert c.get().is_empty()
        print "Passed: test_invalid_messages, N=%s" % N
    
    
    def _test_reassemble(self,N,string_generator):
        # generate and send N messages 
        # numbered 0 through N-1
        # make N get requests and validate that you have all N
        # messages and that count is unchanged
        c=client(HOST,PORT,debug=self.debug)
        count=c.count()
        messages=[]
        return_messages=[]
        for i in range(0,N):
            body=string_generator(self.minlength, self.maxlength, self.minlines, self.maxlines)
            head="_HEAD_START_" + string_generator(self.minlength, 100, 1, 1,) + "_HEAD_END_"
            messages.append(Message(head,body))
        for i in range(0,N):
            c.put(messages[i].head, messages[i].body)
        for i in range(0,N):
            return_messages.append(c.get())
        self.compare_message_sets(return_messages, messages)
        new_count=c.count()
        assert new_count==count

    def compare_message_sets(self,return_messages, original_messages):
        return_messages=set([str(r) for r in return_messages])
        original_messages=set([str(m) for m in original_messages])
        assert return_messages==original_messages, \
            "Return messages not the same as original messages:\n------(not in return)\n%s\n-----(not in original)\n%s " % \
              set_diff_sample(return_messages,original_messages)

    def test_reassemble_basic(self,N):
        # generate and send N messages 
        # numbered 0 through N-1
        # make N get requests and validate that you have all N
        # messages and that count is unchanged
        print "Running: test_reassemble_basic, N=%s" % N
        c=client(HOST,PORT,debug=self.debug)
        count=c.count()
        messages=[]
        return_messages=[]
        for i in range(0,N):
            body="message - %d" % i
            head="head - %d" % i
            messages.append(Message(head,body))
        for i in range(0,N):
            c.put(messages[i].head, messages[i].body)
        for i in range(0,N):
            return_messages.append(c.get())
        self.compare_message_sets(return_messages, messages)
        new_count=c.count()
        assert new_count==count
        print "Passed: test_reassemble_basic, N=%s" % N

    def test_match_basic(self,N):
        # generate and send N messages 
        # numbered 0 through N-1
        # make M match requests and validate that you retrieved
        # only the correct messages
        print "Running: test_match_basic, N=%s" % N
        if N<2: N=2
        M=N/2
        c=client(HOST,PORT,debug=self.debug)
        count=c.count()
        nonmatch_messages=[]
        match_messages=[]
        return_messages=[]
        for i in range(0,M):
            body="message - %d" % i
            head="head - %d" % i
            nonmatch_messages.append(Message(head,body))
        for i in range(0,M):
            msglen=c.put(nonmatch_messages[i].head, nonmatch_messages[i].body)
            assert msglen, "mq return empty message length"
        for i in range(0,M):
            body="message - %d" % i
            head="match" 
            match_messages.append(Message(head,body))
        for i in range(0,M):
            msglen=c.put(match_messages[i].head, match_messages[i].body)
            assert msglen, "mq return empty message length"
        for i in range(0,M):
            return_messages.append(c.match("match"))
        self.compare_message_sets(return_messages, match_messages)
        new_count=c.count()
        assert new_count==count + len(nonmatch_messages)
        print "Passed: test_match_basic, N=%s" % N

    def test_reassemble(self,N):
        print "Running: test_reassemble, N=%s" % N
        self._test_reassemble(N,generateRandomString)
        print "Passed: test_reassemble, N=%s" % N
        return

    def test_reassemble_utf8(self,N):
        print "Running: test_reassemble_utf8, N=%s" % N
        self._test_reassemble(N,generateRandomJapaneseString)
        print "Passed: test_reassemble_utf8, N=%s" % N
        return
            
    def test_empty_gets(self,N):
        # generate and send N messages 
        # numbered 0 through N-1
        # make N get requests and validate that you have all N
        # messages and that count is unchanged
        print "Running: test_emtpy_gets, N=%s" % N
        c=client(HOST,PORT,debug=self.debug)
        count=c.count()
        messages=[]
        return_messages=[]
        for i in range(0,N):
            body=generateRandomString(self.minlength, self.maxlength, self.minlines, self.maxlines)
            head=generateRandomString(10, 100, 1, 1)
            messages.append(Message(head,body))
        for i in range(0,N):
            c.put(messages[i].head, messages[i].body)
        for i in range(0,N):
            return_messages.append(c.get())
        self.compare_message_sets(return_messages, messages)
        new_count=c.count()
        assert new_count==count
        return_messages=[]
        for i in range(0,N):
            return_messages.append(c.get())
        assert len(return_messages)==N
        for m in return_messages: assert m.is_empty()
        print "Passed: test_emtpy_gets, N=%s" % N
    
    def test_count_requests(self,N):
        print "Running: test_count_requests, N=%s" % N
        c=client(HOST,PORT,debug=self.debug)
        count=c.count()
        messages=[]
        return_messages=[]
        for i in range(0,N):
            body=generateRandomString(self.minlength, self.maxlength, self.minlines, self.maxlines)
            head=generateRandomString(10, 100, 1, 1)
            messages.append(Message(head,body))
        for i in range(0,N):
            c.put(messages[i].head, messages[i].body)
        for i in range(0,N):
            cr=c.count()
            assert cr==N-i, "Count request should return %d, but it returned %d." % (cr,N-i)
            return_messages.append(c.get())
        self.compare_message_sets(return_messages, messages)
        new_count=c.count()
        assert new_count==count
        # empty the queue
        while (c.count()): c.get()
        print "Passed: test_count_requests, N=%s" % N

    def run(self,N):
        self.test_invalid_messages(N)
        self.test_match_basic(N)
        self.test_empty_messages(N)
        self.test_count_requests(N)
        self.test_reassemble_basic(N)
        self.test_reassemble(N)
        self.test_empty_gets(N)
        self.test_reassemble_utf8(N)
        #self.test_long_messages(N)
        #self.test_get_extra(N)

# tests
# (1) reassemble
# (2) multiple long messages
# (3) count requests
# (4) gets where there are no messages
# (5) invalid messages

debug=0
N=2000
tmq=test_mq(debug=debug)
tmq.run(N)
