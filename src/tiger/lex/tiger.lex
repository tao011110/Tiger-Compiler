%filenames = "scanner"

 /*
  * Please don't modify the lines above.
  */

 /* You can add lex definitions here. */
 /* TODO: Put your lab2 code here */

%x COMMENT STR IGNORE

%%

 /*
  * Below is examples, which you can wipe out
  * and write regular expressions and actions of your own.
  *
  * All the tokens:
  *   Parser::ID
  *   Parser::STRING
  *   Parser::INT
  *   Parser::COMMA
  *   Parser::COLON
  *   Parser::SEMICOLON
  *   Parser::LPAREN
  *   Parser::RPAREN
  *   Parser::LBRACK
  *   Parser::RBRACK
  *   Parser::LBRACE
  *   Parser::RBRACE
  *   Parser::DOT
  *   Parser::PLUS
  *   Parser::MINUS
  *   Parser::TIMES
  *   Parser::DIVIDE
  *   Parser::EQ
  *   Parser::NEQ
  *   Parser::LT
  *   Parser::LE
  *   Parser::GT
  *   Parser::GE
  *   Parser::AND
  *   Parser::OR
  *   Parser::ASSIGN
  *   Parser::ARRAY
  *   Parser::IF
  *   Parser::THEN
  *   Parser::ELSE
  *   Parser::WHILE
  *   Parser::FOR
  *   Parser::TO
  *   Parser::DO
  *   Parser::LET
  *   Parser::IN
  *   Parser::END
  *   Parser::OF
  *   Parser::BREAK
  *   Parser::NIL
  *   Parser::FUNCTION
  *   Parser::VAR
  *   Parser::TYPE
  */

 /* reserved words */
"array" {adjust(); return Parser::ARRAY;}
 /* TODO: Put your lab2 code here */
"if" {adjust(); return Parser::IF;}
"then" {adjust(); return Parser::THEN;}
"else" {adjust(); return Parser::ELSE;}
"while" {adjust(); return Parser::WHILE;}
"for" {adjust(); return Parser::FOR;}
"to" {adjust(); return Parser::TO;}
"do" {adjust(); return Parser::DO;}
"let" {adjust(); return Parser::LET;}
"in" {adjust(); return Parser::IN;}
"end" {adjust(); return Parser::END;}
"of" {adjust(); return Parser::OF;}
"break" {adjust(); return Parser::BREAK;}
"nil" {adjust(); return Parser::NIL;}
"function" {adjust(); return Parser::FUNCTION;}
"var" {adjust(); return Parser::VAR;}
"type" {adjust(); return Parser::TYPE;}

/* Punctuation symbols */
"," {adjust(); return Parser::COMMA;}
":" {adjust(); return Parser::COLON;}
";" {adjust(); return Parser::SEMICOLON;}
"(" {adjust(); return Parser::LPAREN;}
")" {adjust(); return Parser::RPAREN;}
"[" {adjust(); return Parser::LBRACK;}
"]" {adjust(); return Parser::RBRACK;}
"{" {adjust(); return Parser::LBRACE;}
"}" {adjust(); return Parser::RBRACE;}
"." {adjust(); return Parser::DOT;}
"+" {adjust(); return Parser::PLUS;}
"-" {adjust(); return Parser::MINUS;}
"*" {adjust(); return Parser::TIMES;}
"/" {adjust(); return Parser::DIVIDE;}
"=" {adjust(); return Parser::EQ;}
"<>" {adjust(); return Parser::NEQ;}
"<" {adjust(); return Parser::LT;}
"<=" {adjust(); return Parser::LE;}
">" {adjust(); return Parser::GT;}
">=" {adjust(); return Parser::GE;}
"&" {adjust(); return Parser::AND;}
"|" {adjust(); return Parser::OR;}
":=" {adjust(); return Parser::ASSIGN;}

/* Identifier */
[a-zA-Z][a-zA-Z0-9_]* {adjust(); return Parser::ID;}

/* Integer */
[[:digit:]]+ {adjust(); return Parser::INT;}

/* String */
\" {
  adjust();
  begin(StartCondition__::STR);
  string_buf_ = "";
}

<STR>{
  \" {
    adjustStr();
    setMatched(string_buf_);
    begin(StartCondition__::INITIAL);
    return Parser::STRING;
  }
  \\n {
    adjustStr();
    string_buf_ += "\n";
  }
  \\t {
    adjustStr();
    string_buf_ += "\t";
  }
  \\[[:digit:]]{3} {
    adjustStr();
    std::string str = matched();
    int num = atoi(str.substr(1).c_str());
    string_buf_+= num; 
  }
  \\ {
    adjustStr(); 
    string_buf_ += '\';
  }
  \\\" {
    adjustStr(); 
    string_buf_ += '\"';
  }
  \\\\ {
    adjustStr();
    string_buf_ += "\\";
  }
  \\[ \n\t\f]+\\ {
    adjustStr();
  }
  \\\^[A-Z] {
    adjustStr(); 
    std::string str = matched();
    string_buf_ += str[2] - 'A' + 1;
  }
  . {
    adjustStr();
    std::string str = matched();
    string_buf_ += str;
  }
}

/* COMMENT */
"/*" {
  adjust();
  comment_level_ = 1;
  begin(StartCondition__::COMMENT);
}

<COMMENT>{
  "*/" {
    adjustStr();
    comment_level_--;
    if (comment_level_ == 0) {
      begin(StartCondition__::INITIAL);
    }
  }
  "/*" {
    adjustStr();
    comment_level_++;
  }
  .|\n {
    adjustStr();
  }
}

 /*
  * skip white space chars.
  * space, tabs and LF
  */
[ \t]+ {adjust();}
\n {adjust(); errormsg_->Newline();}

 /* illegal input */
. {adjust(); errormsg_->Error(errormsg_->tok_pos_, "illegal token");}
