#!/bin/bash

declare -a applications=("bubble_sort" "insertion_sort" "merge_sort" "selection_sort")
declare -a column=( "CORRECT" "INCORRECT" "NO_OUTPUT")

for column in "${column[@]}"
do
   echo $column
   for application in "${applications[@]}"
   do
      cat $1  | grep $application | grep $column | wc -l
   done
done

