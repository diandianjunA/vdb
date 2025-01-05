import time
import threading
import requests
import random

# 替换为你的查询向量和参数
def search_vectors(vector, k, url="http://localhost:8080/search"):
    """
    查询与给定向量相似度最高的k个向量

    :param vector: 给定向量
    :param url: 查询向量的URL
    :param k: k个临近向量
    """
    payload = {
        "operation": "search",
        "vector": vector,
        "k": k,
    }
    try:
        response = requests.post(url, json=payload)
        if response.status_code == 200:
            return True
        else:
            print(f"Failed to search vector {vector}. Status code: {response.status_code}, Response: {response.text}")
            return False
    except requests.RequestException as e:
        print(f"Error querying vector {vector}: {e}")
        return False

def query_worker(url, k, vector_length, query_count, success_count):
    """工作线程发送查询请求"""
    for _ in range(query_count):
        vector = [random.random() for _ in range(vector_length)]
        if search_vectors(vector, k, url):
            success_count.append(1)

def test_query_throughput(url, k, vector_length, total_queries, thread_count):
    """
    测试向量数据库查询吞吐

    :param url: 查询接口的URL
    :param k: k个临近向量
    :param vector_length: 向量的长度
    :param total_queries: 总请求数
    :param thread_count: 使用的线程数
    """
    threads = []
    query_per_thread = total_queries // thread_count
    success_count = []

    start_time = time.time()

    for _ in range(thread_count):
        thread = threading.Thread(target=query_worker, args=(url, k, vector_length, query_per_thread, success_count))
        threads.append(thread)
        thread.start()

    for thread in threads:
        thread.join()

    end_time = time.time()

    elapsed_time = end_time - start_time
    qps = len(success_count) / elapsed_time

    print(f"Total time: {elapsed_time:.2f} seconds")
    print(f"Total successful queries: {len(success_count)}")
    print(f"Queries per second (QPS): {qps:.2f}")

if __name__ == "__main__":
    # 配置参数
    VECTOR_LENGTH = 128
    K = 10
    URL = "http://localhost:8080/search"
    TOTAL_QUERIES = 1000
    THREAD_COUNT = 10

    test_query_throughput(URL, K, VECTOR_LENGTH, TOTAL_QUERIES, THREAD_COUNT)