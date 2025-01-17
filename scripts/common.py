import requests
import os
import numpy as np

def generate_random_float_vector(size=128):
    """
    生成一个包含指定数量随机浮点数的向量。
 
    参数:
    size (int): 向量中浮点数的数量，默认为128。
 
    返回:
    numpy.ndarray: 包含随机浮点数的向量。
    """
    return np.random.rand(size).astype(np.float32).tolist()

def list_node(url="http://localhost:8080/listNode"):
    """
    返回集群的节点情况

    :param url: 查询节点的URL
    """
    try:
        response = requests.get(url)
        if response.status_code == 200:
            print(f"listnode successfully.")
            print({response.content})
        else:
            print(f"Failed to listnode. Status code: {response.status_code}, Response: {response.text}")
    except requests.RequestException as e:
        print(f"Error listnode: {e}")

def set_leader(url="http://localhost:8080/setLeader"):
    """
    设置leader

    :param url: 查询节点的URL
    """
    try:
        response = requests.post(url)
        if response.status_code == 200:
            print(f"set leader successfully.")
            print({response.content})
        else:
            print(f"Failed to set leader. Status code: {response.status_code}, Response: {response.text}")
    except requests.RequestException as e:
        print(f"Error set leader: {e}")

def add_follower(nodeId, endpoint, url="http://localhost:8082/addFollower"):
    """
    返回集群的节点情况

    :param nodeId: 节点id
    :param endpoint: 节点ip
    :param url: 查询节点的URL
    """
    payload = {
        "operation": "add_follower",
        "nodeId": nodeId,
        "endpoint": endpoint
    }
    try:
        response = requests.post(url, json=payload)
        if response.status_code == 200:
            print(f"add follower successfully.")
            print({response.content})
        else:
            print(f"Failed to add follower. Status code: {response.status_code}, Response: {response.text}")
    except requests.RequestException as e:
        print(f"Error add follower: {e}")

def insert(vector, id, url="http://localhost:9090/insert"):
    """
    将向量数据发送到服务器

    :param vectors: 要插入的向量（numpy array）
    :param url: 插入向量的URL
    :param index_type: 索引类型
    """
    payload = {
        "operation": "insert",
        "object": {
            "vector": vector,
            "id": id
        },
    }
    try:
        response = requests.post(url, json=payload)
        if response.status_code == 200:
            print(f"Inserted vector ID {id} successfully.")
        else:
            print(f"Failed to insert vector ID {id}. Status code: {response.status_code}, Response: {response.text}")
    except requests.RequestException as e:
        print(f"Error inserting vector ID {id}: {e}")

if __name__ == "__main__":
    # list_node()
    # set_leader()
    # add_follower(4, '127.0.0.1:9093')
    vector = generate_random_float_vector()
    insert(vector, 10)