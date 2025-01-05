import os
import subprocess
import requests
import numpy as np

def create_cluster():
    input("start vdb_server")
    print("vdb_server node has started")
    # 集群添加节点
    post({"operation": "add_follower","nodeId": 4, "endpoint": "192.168.6.202:9093"}, "http://192.168.6.201:8082/addFollower")
    print("raft has been created")
    get({}, "http://192.168.6.201:8082/listNode")
    input("start master_server")
    print("master_server node has started")
    # 添加集群元数据
    post({"instanceId": "instance2", "nodeId": "node3", "url": "http://192.168.6.201:8082", "role": 0, "type": 1}, "http://localhost:6060/addNode")
    post({"instanceId": "instance2", "nodeId": "node4", "url": "http://192.168.6.202:9092", "role": 1, "type": 2}, "http://localhost:6060/addNode")
    # 查看所有节点信息
    get({"instanceId": "instance2"}, "http://localhost:6060/getInstance")
    # 启动代理服务器
    input("start proxy_server")
    print("proxy_server node has started")
    # input()
    # 查看拓扑结构
    get({}, "http://localhost:6061/topology")


def post(payload, url):
    """
    POST请求方法

    :param payload: 请求参数
    :param url: 请求url
    """
    try:
        response = requests.post(url, json=payload)
        if response.status_code == 200:
            print(f"post successfully.")
            print({response.content})
        else:
            print(f"Failed to post. Status code: {response.status_code}, Response: {response.text}")
    except requests.RequestException as e:
        print(f"Error post: {e}")

def get(params, url):
    try:
        response = requests.get(url, params=params)
        if response.status_code == 200:
            print(f"get successfully.")
            print({response.content})
        else:
            print(f"Failed to get. Status code: {response.status_code}, Response: {response.text}")
    except requests.RequestException as e:
        print(f"Error post: {e}")
 
def generate_random_float_vector(size=128):
    """
    生成一个包含指定数量随机浮点数的向量。
 
    参数:
    size (int): 向量中浮点数的数量，默认为128。
 
    返回:
    numpy.ndarray: 包含随机浮点数的向量。
    """
    return np.random.rand(size).astype(np.float32).tolist()

def test1():
    vector = generate_random_float_vector()
    post({"operation": "insert", "object": {"id": 6, "vector": vector, "int_field": 49}}, "http://localhost:6061/insert")
    post({"operation": "search", "vector": vector, "k": 5}, "http://localhost:6061/search")
    post({"operation": "query", "id": 6,}, "http://localhost:6061/query")

# 主函数
def main():
    create_cluster()
    test1()

 
if __name__ == "__main__":
    main()