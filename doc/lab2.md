<h3>how I handle comments</h3>

 	I use the variable `comment_level` to declare the level where the comment in, because the comment may be nested. 

​	When reading "/*",  I  start the`StartCondition__::COMMENT` start condition and set the `comment_level` as one.  

​	Then every time reading "/*",  I increase the `comment_level` by one.

​	And when reading "*/", I reduce the `comment_level` by one.  Once the `comment_level` equals to zero, which means the comment is end, I switch the state from `StartCondition__::COMMENT` to the`StartCondition__::INITIAL`.

​	As for other characters, I just change the `char_pos_`.



<h3> how I handle strings;</h3>

​	When reading one quotes (“) ,  I  start the`StartCondition__::STR` start condition.

​	Then when reading some escape characters, I will add them into the `string_buf_` in the right form.

 	As soon as reading another quotes (“) , which means the string is end, I switch the state from `StartCondition__::STR` to the`StartCondition__::INITIAL`.



<h3> error handling</h3>

When facing illegal input,  I will report "illegal token"!



<h3> end-of-file handling</h3>

No need to handle that in the lexer. The end-of-file will not become the input into the lexer.



<h3>other interesting features of your lexer</h3>

To be frank, there is nothing interesting. My lexer is just a normal one. 

