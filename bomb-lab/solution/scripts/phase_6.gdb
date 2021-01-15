define show_linked_list
  set $node=0x0804b26c
  printf "|  i  | value |\n"
  printf "| --- | ----- |\n"
  while ($node != 0)
    printf "| %d | %d |\n", *(int*)($node + 4), *(int*)($node)
    set $node=*(long*)($node + 8)
  end
end
show_linked_list
