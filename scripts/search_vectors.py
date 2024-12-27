import requests
import os

def query_vectors(id, url="http://localhost:6061/query", index_type="HNSWFLAT"):
    """
    根据id查询向量

    :param id: 要查询的向量id
    :param url: 查询向量的URL
    :param index_type: 索引类型
    """
    payload = {
        "operation": "query",
        "id": id,
    }
    try:
        response = requests.post(url, json=payload)
        if response.status_code == 200:
            print(f"search vector ID {id} successfully.")
            print({response.content})
        else:
            print(f"Failed to search vector ID {id}. Status code: {response.status_code}, Response: {response.text}")
    except requests.RequestException as e:
        print(f"Error inserting vector ID {id}: {e}")

def search_vectors(vector, k, url="http://localhost:8080/search", index_type="FLAT"):
    """
    根据id查询向量

    :param id: 要查询的向量id
    :param url: 查询向量的URL
    :param index_type: 索引类型
    """
    payload = {
        "operation": "search",
        "vector": vector,
        "k": k,
    }
    try:
        response = requests.post(url, json=payload)
        if response.status_code == 200:
            print(f"search vector {vector} successfully.")
            print({response.content})
        else:
            print(f"Failed to search vector {vector}. Status code: {response.status_code}, Response: {response.text}")
    except requests.RequestException as e:
        print(f"Error inserting vector {vector}: {e}")

if __name__ == "__main__":
    # query_vectors(4)
    vector = [1]
    search_vectors(vector, k=5)