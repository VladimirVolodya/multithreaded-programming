import sys


even = True
mean_val = 0
max_val = 0
for line in sys.stdin:
    val = int(line)
    if even:
        max_val += val
    else:
        mean_val += val
    even = not even
print(str(max_val / 10) + ' ' + str(mean_val / 10))

