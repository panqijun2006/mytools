# coding=utf-8
# 代码来源：https://blog.csdn.net/qq_53500156/article/details/123315789 

graph = {
    "A":["B","C"],
    "B":["A","C","D"],
    "C":["A","B","D","E"],
    "D":["B","C","E","F"],
    "E":["C","D"],
    "F":["D"]
}

def bfs(graph, star):
    seen = set()
    stack = []
    stack.append(star)
    seen.add(star)
    scan = []
    
    while len(stack):
        cur = stack[0]   # bfs是取栈底值（排在前面的），dfs是取栈顶（排在后面的）
        del stack[0]
        vers = graph[cur]
        for ver in vers:
            if ver not in seen:
                stack.append(ver)
                seen.add(ver)
        scan.append(cur)
    return scan

print("bfs test: ")
scan = bfs(graph, "A")
print(" -> ".join(scan))
