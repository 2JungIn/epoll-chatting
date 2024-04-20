import os

log_dir = "log"
standard = "client(0).txt"

log_list = os.listdir(log_dir)
log_list.remove(standard)

def read_file(path):
    msgs = []

    with open(path, "r", encoding="utf8") as f:
        while True:
            msg = f.readline()
        
            if msg == "":
                break

            msgs.append(f.readline())

    return msgs

ref_msgs = read_file(f"{log_dir}/{standard}")

equal_cnt = 0
for log in log_list:
    result = True
    msgs = read_file(f"{log_dir}/{log}")
    
    # 파일 비교
    for msg1, msg2 in zip(ref_msgs, msgs):
        result &= True if msg1 == msg2 else False

    if result == True:
        equal_cnt = equal_cnt + 1
    else:
        print(f"{standard} != {log} -> {result}")

print(f"equal cnt: {equal_cnt}")