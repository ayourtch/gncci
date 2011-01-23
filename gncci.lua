require "ydns"

function gncci_socket(domain, type, protocol)
  -- print("Socket!")
  return o.socket(domain,type,protocol)
end

function gncci_connect(sockfd, sa)
  -- print("Connect!", sa.af, sa.addr, sa.port)
  return o.connect(sockfd, sa)
end

function gncci_send(sockfd, buf, flags)
  -- print("Send:", sockfd, buf)
  return o.send(sockfd, buf, flags)
end

function gncci_sendto(sockfd, buf, flags, sa)
  -- print("Sendto", sockfd)
  return o.sendto(sockfd, buf, sa)
end

function gncci_recv(sockfd, len, flags)
  -- print("Recv:", sockfd)
  local ret = o.recv(sockfd, len, flags)
  return ret
end

function gncci_recvfrom(sockfd, len, flags)
  local from, ret = o.recvfrom(sockfd, len, flags)
  print("Recvfrom:", from.addr, from.port)
  return from, ret
end


function gncci_init()
  print("Hello!")
end
