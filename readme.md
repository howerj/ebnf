# EBNF 

| Project   | EBNF Parser Generator              |
| --------- | ---------------------------------- |
| Author    | Richard James Howe                 |
| Copyright | 2019 Richard James Howe            |
| License   | MIT                                |
| Email     | howe.r.j.89@gmail.com              |
| Website   | <https://github.com/howerj/ebnf>   |

## Goals

* [ ] Determine the minimal instruction set
* [ ] Either make a front end in C that can build up an [EBNF][] grammar
  or use the nascent Virtual Machine and hand compile a program that
  can.
* [ ] Compile an EBNF grammar to a virtual machine
* [ ] Use the virtual machine to generate an Abstract Syntax Tree built up of
  nodes.
* [ ] Extend virtual machine so it can optionally be used as a generic virtual
  machine, the generic instructions should have the option of being turned
  on or off.
* Add a few extension to the EBNF grammar
  - [ ] For compiling arbitrary instructions specific to this virtual machine
  - [ ] Have some predefined functions for checking for:
    - [ ] Numbers, alphabetic characters, character ranges, etcetera.
  - [ ] Allow regular expressions to be used?
* [ ] Do we *need* a stack (if so an explicit one should be used not the C stack),
  or can we use the Nodes we are generating to store the information we need?
* [ ] Turn into a library, with a compiler front end for compiling the EBNF grammar
  into executable virtual machine code and the virtual machine itself.
* [ ] Make the project suitable for embedded use:
  - [ ] Allow a custom allocator to be used with the virtual machine.
  - [ ] Allow tree depth and execution instruction count maximums to be specified
  - [ ] Have a minimal dependency (in the virtual machine) on the C library,
  preferable using none of the 'FILE\*' related functions. 
  - [ ] Deal with Out Of Memory conditions correctly.
* [ ] Add manual page

## References

* [doc.tgz][]
* <https://en.wikipedia.org/wiki/Extended_Backus%E2%80%93Naur_form>
* <https://en.wikipedia.org/wiki/PL/0>
* <https://en.wikipedia.org/wiki/P-code_machine>
* <https://en.wikipedia.org/wiki/Recursive_descent_parser>
* <https://rosettacode.org/wiki/Parse_EBNF>
* <http://www.iro.umontreal.ca/~felipe/IFT2030-Automne2002/Complements/tinyc.c>

## Appendix

From <https://en.wikipedia.org/wiki/Extended_Backus%E2%80%93Naur_form>, the
[EBNF][] grammar for [EBNF][]:

	letter = "A" | "B" | "C" | "D" | "E" | "F" | "G"
	       | "H" | "I" | "J" | "K" | "L" | "M" | "N"
	       | "O" | "P" | "Q" | "R" | "S" | "T" | "U"
	       | "V" | "W" | "X" | "Y" | "Z" | "a" | "b"
	       | "c" | "d" | "e" | "f" | "g" | "h" | "i"
	       | "j" | "k" | "l" | "m" | "n" | "o" | "p"
	       | "q" | "r" | "s" | "t" | "u" | "v" | "w"
	       | "x" | "y" | "z" ;
	digit = "0" | "1" | "2" | "3" | "4" | "5" | "6" | "7" | "8" | "9" ;
	symbol = "[" | "]" | "{" | "}" | "(" | ")" | "<" | ">"
	       | "'" | '"' | "=" | "|" | "." | "," | ";" ;
	character = letter | digit | symbol | "_" ;
	 
	identifier = letter , { letter | digit | "_" } ;
	terminal = "'" , character , { character } , "'" 
		 | '"' , character , { character } , '"' ;
	 
	lhs = identifier ;
	rhs = identifier
	     | terminal
	     | "[" , rhs , "]"
	     | "{" , rhs , "}"
	     | "(" , rhs , ")"
	     | rhs , "|" , rhs
	     | rhs , "," , rhs ;

	rule = lhs , "=" , rhs , ";" ;
	grammar = { rule } ;

[doc.tgz]: doc.tgz
[EBNF]: https://en.wikipedia.org/wiki/Extended_Backus%E2%80%93Naur_form

<style type="text/css">body{margin:40px auto;max-width:850px;line-height:1.6;font-size:16px;color:#444;padding:0 10px}h1,h2,h3{line-height:1.2}table {width: 100%; border-collapse: collapse;}table, th, td{border: 1px solid black;}code { color: #091992; } </style>
