import threading
import random
import time
import requests
import numpy as np


def search(vector, k, url="http://localhost:6061/search"):
    payload = {
        "operation": "search",
        "vector": vector,
        "k": k,
    }
    try:
        start = int(time.perf_counter() * 1e9)
        response = requests.post(url, json=payload)
        end = int(time.perf_counter() * 1e9)
        print("开始时间: {}, 结束时间: {}", start, end)
        if response.status_code == 200:
            print(f"Search vector {vector[:5]}... successfully.")
        else:
            print(f"Failed to search vector {vector[:5]}... Status code: {response.status_code}, Response: {response.text}")
    except requests.RequestException as e:
        print(f"Error searching vector {vector[:5]}...: {e}")

# 插入请求函数
def insert(vector, id, url="http://localhost:6061/insert"):
    payload = {
        "operation": "insert",
        "object": {
            "vector": vector,
            "id": id
        },
    }
    try:
        start = int(time.perf_counter() * 1e9)
        response = requests.post(url, json=payload)
        end = int(time.perf_counter() * 1e9)
        print("开始时间: {}, 结束时间: {}", start, end)
        if response.status_code == 200:
            print(f"Inserted vector ID {id} successfully.")
        else:
            print(f"Failed to insert vector ID {id}. Status code: {response.status_code}, Response: {response.text}")
    except requests.RequestException as e:
        print(f"Error inserting vector ID {id}: {e}")

# 测试线程函数
def run_test(type, vector_dim, search_k):
    vector = np.random.random(vector_dim).tolist()
    if type:
        # 查询操作
        search(vector, search_k)
    else:
        # 插入操作
        vector_id = random.randint(1, 10000)
        insert(vector, vector_id)

def main():
    type = True
    vector_dim = 128
    search_k = 5
    run_test(type, vector_dim, search_k)

if __name__ == "__main__":
    main()