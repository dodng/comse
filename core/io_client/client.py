import httplib, urllib

params = urllib.urlencode({'number': 12524, '&type': 'text', 'action': "start--dfsdfasdfsssssssssssssss"
		"asdddddddddddddddddddddddddwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwww---done"
		"how"})

headers = {"Content-type": "application/x-www-form-urlencoded", \
	"Accept": "text/plain"}

conn = httplib.HTTPConnection("127.0.0.1",8807)
conn.request("POST", "", params, headers)
response = conn.getresponse()
print response.status, response.reason
data = response.read()
print data
conn.close()
