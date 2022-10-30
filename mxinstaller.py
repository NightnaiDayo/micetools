from pprint import pformat, pprint
import socket

s = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
s.connect(("127.0.0.1", 12121))


slots = [
    "original0",
    "original1",
    "patch0",
    "patch1",
    "os",
    "app_data",
    "originalf",
    "originalb",
    "patchf",
    "patchb",
]


"""
slot 0: OS
os                   result=success&status=complete&id=AAS0&version=4524545&time=00000000000000&segcount=1745&segsize=262144&hw=AAS&instant=0&osver=4524545&ossegcount=0&orgtime=00000000000000&orgversion=4524545

slot 1: original0
original0            result=success&status=complete&id=SDEY&version=65633&time=20181029150736&segcount=65082&segsize=262144&hw=AAS&instant=0&osver=4524545&ossegcount=1745&orgtime=20181029150736&orgversion=65633

slot 2: ??

slot 3: ??

slot 4: ??


original1            result=error&status=error&code=40
patch0               result=success&status=empty
patch1               result=success&status=complete&id=SDEY&version=65634&time=20181116190448&segcount=173&segsize=262144&hw=AAS&instant=0&osver=0&ossegcount=0&orgtime=20181029150736&orgversion=65633
app_data             result=invalid_parameter
originalf            result=success&status=complete&id=SDEY&version=65633&time=20181029150736&segcount=65082&segsize=262144&hw=AAS&instant=0&osver=4524545&ossegcount=1745&orgtime=20181029150736&orgversion=65633
originalb            result=error&status=error&code=40
patchf               result=success&status=complete&id=SDEY&version=65634&time=20181116190448&segcount=173&segsize=262144&hw=AAS&instant=0&osver=0&ossegcount=0&orgtime=20181029150736&orgversion=65633
patchb               result=success&status=empty
"""
def recv_pcp():
    resp = s.recv(4096).decode().strip()
    if resp == "?":
        raise Exception("PCP Failed")
    parts = resp.split("&")
    return {i.split("=")[0]: i.split("=")[1] for i in parts}
def send_pcp(request, **kwargs):
    req = f"request={request}"
    if kwargs:
        req += "&" + "&".join(f"{i}={kwargs[i]}" for i in kwargs)
    assert s.recv(4096) == b">"
    s.send(req.encode() + b"\r\n")
def pcp(request, **kwargs):
    send_pcp(request, **kwargs)
    ret = recv_pcp()
    if ret["result"] != "success" or ret["response"] != request:
        raise Exception(f"PCP Failed:\n{pformat(ret)}")
    del ret["response"]
    del ret["result"]
    return ret

print("Query slots:")
for slot in slots:
    assert s.recv(4096) == b">"
    s.send(f"request=query_slot_status&slot={slot}\r\n".encode())
    print(slot.ljust(20), s.recv(4096).decode().strip())

    # assert s.recv(4096) == b">"
    # s.send(f"request=check&slot={slot}\r\n".encode())
    # print(slot.ljust(20), s.recv(4096).decode().strip())

print("-" * 10)

if pcp("query_semaphore_status")["semaphore"] != "1":
    print("Unable to get semaphore!")
    quit()

gsem = pcp("get_semaphore")
semid = gsem["semid"]
try:
    spd = pcp("query_spd")
    br = pcp("query_br")
    bootslot = pcp("query_sbr_bootslot")
    print("=== SPD ===")
    order = spd.pop("order")
    print(f"--- Order: {order}")
    spd = list(spd.items())
    spd.sort(key=lambda x: int(x[1]))
    for i in spd:
        # Values here are uk3 * block_size
        print(f" - {(i[0] + ':').ljust(10)} {int(i[1]):016x}")
    print(f"--- Bootslot: {bootslot['bootslot']}")
    print("=== BR ===")
    pprint(br)
    quit()

    assert s.recv(4096) == b">"
    s.send((
        b"request=check&"
        b"slot=original0&"
        b"segoffset=0&"
        b"segcount=100000&"
        b"force=1&"
        b"semid=" + sem + b""
        b"\r\n"
    ))
    print(s.recv(4096).decode().strip())

    assert s.recv(4096) == b">"
    s.send((
        b"request=release_semaphore&"
        b"semid=" + sem + b""
        b"\r\n"
    ))
    print(s.recv(4096).decode().strip())

    quit()

    # assert s.recv(4096) == b">"
    # s.send(b"request=set_sbr_bootslot&bootslot=cd\r\n")
    # print(s.recv(4096).decode().strip())
    assert s.recv(4096) == b">"
    s.send(b"request=query_application_status\r\n")
    print(s.recv(4096).decode().strip())
    assert s.recv(4096) == b">"
    s.send(b"request=query_br\r\n")
    print(s.recv(4096).decode().strip())
    assert s.recv(4096) == b">"
    s.send((
        b"request=appdata&"
        b"slot=os&"
        b"semid=" + sem + b"&"
        b"\r\n"
    ))
    # s.send((
    #     b"request=install&"
    #     b"slot=patch0&"
    #     b"segoffset=3&"
    #     b"id=SDEY&"
    #     b"version=010061&"
    #     b"time=20181029150736&"
    #     b"segcount=fe3a&"
    #     b"segsize=040000&"
    #     b"hw=AAS&"
    #     b"instant=3&"
    #     b"osver=450a01&"
    #     b"ossegcount=06d1&"
    #     b"orgtime=20181029150736&"
    #     b"orgversion=10061&"
    #     b"semid=" + sem + b"&"
    #     b"\r\n"
    # ))
    print(s.recv(4096).decode().strip())
finally:
    pcp("release_semaphore", semid=semid)