#!/usr/bin/env python3

def zipflaw_cdf(k, n):
    a = 0.0
    b = 0.0
    for i in range(k):
        a += 1.0 / (i + 1)
    for i in range(n):
        b += 1.0 / (i + 1)
    return a/b

print(zipflaw_cdf(100, 1000))
print(zipflaw_cdf(200, 2000))
print(zipflaw_cdf(400, 4000))
