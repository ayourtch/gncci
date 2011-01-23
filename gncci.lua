local ydns = require "ydns"

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

function gncci_recvfrom(sockfd, len, flags)
  local from, ret = o.recvfrom(sockfd, len, flags)
  -- print("Recvfrom:", from.addr, from.port)
  if from.port == 53 then
    local dns = ydns.decode_reply(ret)
    print_dns(dns)
  end
  return from, ret
end


function gncci_init()
  print("Hello!")
end
