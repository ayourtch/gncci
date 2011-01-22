require "ydns"

function gncci_socket(domain, type, protocol)
  print("Socket!")
  return o.socket(domain,type,protocol)
end

function gncci_connect(sockfd, addr)
  print("Connect!", addr.af, addr.addr, addr.port)
  return o.connect(sockfd, addr)
end

function gncci_send(sockfd, buf, flags)
  print("Send:", sockfd, buf)
  return o.send(sockfd, buf, flags)
end

function gncci_init()
  print("Hello!")
end
