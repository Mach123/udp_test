#!/bin/python3

import sys
import numpy as np

#import matplotlib as mpl
import matplotlib.pyplot as plt


def reg1dim(x, y):
    n = len(x)
    a = ((np.dot(x, y) - y.sum() * x.sum() / n) / ((x ** 2).sum() - x.sum()**2 / n))
    b = (y.sum() - a * x.sum()) / n
    return a, b



def main_body():
    x_l = []
    y_l = []

    #with open('testlap5.txt', 'r') as file:
    filename = sys.argv[1]
    with open(filename, 'r') as file:
        for line in file:
            if line.startswith('L'):
                str = line.split()
#                print(str[2], str[7])
                x_l.append(int(str[2]))
                y_l.append(int(str[7]))

    x = np.array(x_l)
    y = np.array(y_l)


    a, b = reg1dim(x, y)
    print(a, b)
    plt.scatter(x, y, color="k")
    plt.plot([0, x.max()], [b, a * x.max() + b])

#    plt.plot(x, y)
    plt.show()




def main():
    main_body()

main()