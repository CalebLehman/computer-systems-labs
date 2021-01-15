define show_passwords
  printf "| First Integer | Character | Second Integer |\n"
  printf "| ------------- | --------- | -------------- |\n"
  set $i=0
  while ($i < 8)
    printf "| %d | %c | %d |\n", $i, *(char*)(*(int*)($i*4 + 0x080497e8)+1), *(short*)(*(int*)($i*4 + 0x080497e8)+5)
    set $i+=1
  end
end
show_passwords
