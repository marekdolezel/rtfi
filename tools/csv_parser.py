#!/usr/bin/python3

import argparse
import pandas

class SortednessClass:
    a = 0
    b = 0
    def __init__ (self, a, b):
        self.a = a 
        self.b = b


if __name__ == "__main__":
    parser = argparse.ArgumentParser(description='Process some integers.')
    parser.add_argument('path_csv', help='path to csv file')
    args = parser.parse_args()

    try:
        csvfile = open(args.path_csv, newline='')
        df = pandas.read_csv(csvfile, ',')
        app_names = ["bubble_sort", "insertion_sort", "merge_sort", "selection_sort" ]

        
        sorted_graph1_str = "Sorted "
        notsorted_graph1_str = "Notsorted "

        notsorted1_graph2_str = "Notsorted1 "
        notsorted2_graph2_str = "Notsorted2 "
        notsorted3_graph2_str = "Notsorted3 "

        sortedness_class1 = SortednessClass(1, 1651)
        sortedness_class2 = SortednessClass(1652, 3302)
        sortedness_class3 = SortednessClass(3303, 4950)

        print(sortedness_class1.a, sortedness_class1.b)


        for app in app_names:
            sorted_rows = df.loc[(df[" application_name"] == app) & (df[" rtfi_is_array_sorted"] == 0)].shape[0]
            sorted_graph1_str += str(sorted_rows) + " "
           
            notsorted_rows = df.loc[(df[" application_name"] == app) & (df[" rtfi_is_array_sorted"] != 0)].shape[0]
            notsorted_graph1_str += str(notsorted_rows) + " "


            notsorted1_rows = df.loc[(df[" application_name"] == app) & (df[" rtfi_is_array_sorted"] >= sortedness_class1.a) & (df[" rtfi_is_array_sorted"] <= sortedness_class1.b)].shape[0]
            notsorted1_graph2_str += str(notsorted1_rows) + " "
            
            notsorted2_rows = df.loc[(df[" application_name"] == app) & (df[" rtfi_is_array_sorted"] >= sortedness_class2.a) & (df[" rtfi_is_array_sorted"] <= sortedness_class2.b)].shape[0]
            notsorted2_graph2_str += str(notsorted2_rows) + " "
            
            notsorted3_rows = df.loc[(df[" application_name"] == app) & (df[" rtfi_is_array_sorted"] >= sortedness_class3.a) & (df[" rtfi_is_array_sorted"] <= sortedness_class3.b)].shape[0]
            notsorted3_graph2_str += str(notsorted3_rows) + " "

        print("--> graph1")
        print("Category b1 b2 b3 b4")
        print(sorted_graph1_str)
        print(notsorted_graph1_str)
        print("--> graph2")
        print("Category b1 b2 b3 b4")
        print(notsorted1_graph2_str)
        print(notsorted2_graph2_str)
        print(notsorted3_graph2_str)

    except IOError:
        print("Could not open file", args.path_csv)
        exit(1)

    exit(0)
