grammar Formula;

main
    : expr EOF
    ;

expr
    : '(' expr ')'              # Parens
    | (ADD | SUB) expr          # UnaryOp
    | expr (MUL | DIV) expr     # BinaryOp
    | expr (ADD | SUB) expr     # BinaryOp
    | CELL                      # Cell
    | NUMBER                    # Literal
    ;

// ---- Ëèòåğàëû ----

// ×èñëà
fragment UINT : [0-9]+ ;
fragment EXPONENT : [eE] [+-]? UINT ;
NUMBER
    : UINT EXPONENT?
    | UINT? '.' UINT EXPONENT?
    ;

// ß÷åéêè: òîëüêî çàãëàâíûå áóêâû è öèôğû, íà÷èíàåòñÿ ñ áóêâû
CELL
    : [A-Z]+ [0-9]+
    ;

// Îïåğàòîğû
ADD : '+' ;
SUB : '-' ;
MUL : '*' ;
DIV : '/' ;

// ---- Èñêëş÷àåì íåäîïóñòèìûå ïîñëåäîâàòåëüíîñòè ----

// Îòêëîíÿåì ìíîæåñòâåííûå çíàêè ïîäğÿä (++, --, +-, etc.)
INVALID_OPERATOR_SEQUENCE
    : (ADD | SUB) (ADD | SUB)+
    -> skip
    ;

// Îòêëîíÿåì íåçàêğûòûå ñêîáêè èëè ìóñîğ
UNEXPECTED_CHAR
    : .
    -> skip
    ;

WS
    : [ \t\n\r]+ -> skip ;