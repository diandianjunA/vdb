import numpy as np
import requests
import os

def read_vectors_from_file(file_name, vector_dim=100):
    """
    从二进制文件中读取向量数据

    :param file_name: 输入文件名
    :param vector_dim: 向量的维度
    :return: 读取的向量数据（numpy array）
    """
    with open(file_name, 'rb') as f:
        data = f.read()
    return np.frombuffer(data, dtype=np.float32).reshape(-1, vector_dim)

def post_vector_to_server(vectors, url="http://localhost:8080/insert", index_type="HNSW"):
    """
    将向量数据发送到服务器

    :param vectors: 要插入的向量（numpy array）
    :param url: 插入向量的URL
    :param index_type: 索引类型
    """
    for i, vector in enumerate(vectors):
        payload = {
            "vectors": vector.tolist(),
            "id": i + 1,  # 自定义ID，从1开始
            "index_type": index_type
        }
        try:
            response = requests.post(url, json=payload)
            if response.status_code == 200:
                print(f"Inserted vector ID {i + 1} successfully.")
            else:
                print(f"Failed to insert vector ID {i + 1}. Status code: {response.status_code}, Response: {response.text}")
        except requests.RequestException as e:
            print(f"Error inserting vector ID {i + 1}: {e}")

def post_vectors_batch_to_server(vectors, batch_size=100, url="http://localhost:8080/insert_batch", index_type="HNSW"):
    """
    将向量数据批量发送到服务器

    :param vectors: 要插入的向量（numpy array）
    :param batch_size: 每批次插入的向量数
    :param url: 插入向量的URL
    :param index_type: 索引类型
    """
    total_vectors = len(vectors)
    for start_idx in range(0, total_vectors, batch_size):
        end_idx = min(start_idx + batch_size, total_vectors)
        batch_vectors = vectors[start_idx:end_idx]
        batch_ids = list(range(start_idx + 1, end_idx + 1))  # 自定义ID，从1开始
        
        payload = {
            "vectors": batch_vectors.tolist(),
            "ids": batch_ids,
            "index_type": index_type
        }
        
        try:
            response = requests.post(url, json=payload)
            if response.status_code == 200:
                print(f"Inserted batch {start_idx + 1} to {end_idx} successfully.")
            else:
                print(f"Failed to insert batch {start_idx + 1} to {end_idx}. Status code: {response.status_code}, Response: {response.text}")
        except requests.RequestException as e:
            print(f"Error inserting batch {start_idx + 1} to {end_idx}: {e}")

if __name__ == "__main__":
    # 向量文件配置
    file_names = [
        # "vector_5MB.bin",
        "vector_50MB.bin",
        # "vector_100MB.bin",
        # "vector_500MB.bin"
    ]
    
    # # HTTP 目标 URL
    # target_url = "http://localhost:8080/insert"
    # HTTP 目标 URL
    target_url = "http://localhost:8080/insert_batch"
    
    # # 逐个文件处理
    # for file_name in file_names:
    #     if not os.path.exists(file_name):
    #         print(f"File {file_name} does not exist. Skipping.")
    #         continue
        
    #     print(f"Processing file: {file_name}")
    #     vectors = read_vectors_from_file(file_name)
    #     print(f"Read {len(vectors)} vectors from {file_name}.")
        
    #     post_vector_to_server(vectors, url=target_url)

    # 批量大小
    batch_size = 100000  # 可根据实际情况调整批量大小
    
    # 逐个文件处理
    for file_name in file_names:
        if not os.path.exists(file_name):
            print(f"File {file_name} does not exist. Skipping.")
            continue
        
        print(f"Processing file: {file_name}")
        vectors = read_vectors_from_file(file_name)
        print(f"Read {len(vectors)} vectors from {file_name}.")
        
        post_vectors_batch_to_server(vectors, batch_size=batch_size, url=target_url)