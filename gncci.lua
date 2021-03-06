-- local ydns = require "ydns"

-- man 2 socket to know what parameters and return values are
function gncci_socket(domain, type, protocol)
  print("Socket!")
  return o.socket(domain,type,protocol)
end

-- nothing to do with the API, just a fun function
function clamp_mss(sockfd)
  o.setsockopt(sockfd, 6, 2, "\000\004\000\000")
end

-- man 2 connect, except "sa" does not need a len,
-- it's a table with three members:
--   sa: address family
--   addr: printable representation of the address
--   port: the port number

function gncci_connect(sockfd, sa)
  print("This is a Connect!", sa.af, sa.addr, sa.port)
  -- turn on the interception of "read"
  -- o.intercept_read(true)

  -- you can clamp the MSS here, if you want.
  -- clamp_mss(sockfd)
  return o.connect(sockfd, sa)
end

-- man 2 send, pretty much
function gncci_send(sockfd, buf, flags)
  print("Send:", sockfd, buf)
  return o.send(sockfd, buf, flags)
end

function gncci_write(sockfd, buf)
  print("Write:", sockfd, buf)
  return o.write(sockfd, buf)
end

-- man 2 sendto, see the connect above for 
-- info on what "sa" is.
function gncci_sendto(sockfd, buf, flags, sa)
  print("Sendto", sockfd)
  return o.sendto(sockfd, buf, sa)
end

-- receive up to len from sockfd,
-- and return the value you have received
function gncci_recv(sockfd, len, flags)
  print("Recv:", sockfd)
  local ret = o.recv(sockfd, len, flags)
  print("Recv result", ret)
  return ret
end

function gncci_read(sockfd, len)
  print("Read:", sockfd)
  local ret = o.read(sockfd, len)
  return ret
end

-- This one is fun if you do use ydns to print the responses
function print_dns(dns)
  for k,v in pairs(dns) do
    if type(v) == "table" then
      print(k, "======>")
      for k1,v1 in pairs(v) do
	print(":", k1, v1)
      end
    else
      print(k,v)
    end
  end
end

-- try to get up to len bytes on sockfd,
-- returns two values:
--   from: sockaddr table (af, addr, port)
--   ret: the data received
function gncci_recvfrom(sockfd, len, flags)
  local from, ret = o.recvfrom(sockfd, len, flags)
  -- print("Recvfrom:", from.addr, from.port)
  if from.port == 53 then
    -- local dns = ydns.decode_reply(ret)
    -- print_dns(dns)
  end
  return from, ret
end

-- fds is an array with tables
-- each table has "fd", "events" and "revents" members.
-- the semantics of which is the same as in man 2 poll
function gncci_poll(fds, timeout)
  -- print("Doing poll", #fds, timeout); for i=1,#fds do print("ifds["..i.."]", fds[i].fd, fds[i].events, fds[i].revents) end
  local ret, fds = o.poll(fds, timeout)
  -- print("poll:", fds, ret); for i=1,#fds do print("ofds["..i.."]", fds[i].fd, fds[i].events, fds[i].revents) end
  return ret, fds
end

-- rfds, wfds, efds are arrays full of file descriptor #s
-- tsec and tusec are the pieces of the timeval struct
-- you're supposed to return all five.
function gncci_select(rfds, wfds, efds, tsec, tusec)
  print("Select!", #rfds, #wfds, #efds, tsec, tusec)
  local a_rfds, a_wfds, a_efds, a_tsec, a_tusec = o.select(rfds, wfds, efds, tsec, tusec)
  return a_rfds, a_wfds, a_efds, a_tsec, a_tusec
end

function gncci_setsockopt(sockfd, level, optname, optval)
  local ret = o.setsockopt(sockfd, level, optname, optval)
  print("Setsockopt", sockfd, level, optname, optval);
  return ret;
end

-- this one gets called upon init of the library
function gncci_init()
end
