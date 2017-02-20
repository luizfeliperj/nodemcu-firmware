/*
** 2000-05-29
**
** The author disclaims copyright to this source code.  In place of
** a legal notice, here is a blessing:
**
**    May you do good and not evil.
**    May you find forgiveness for yourself and forgive others.
**    May you share freely, never taking more than you give.
**
*************************************************************************
** Driver template for the LEMON parser generator.
**
** The "lemon" program processes an LALR(1) input grammar file, then uses
** this template to construct a parser.  The "lemon" program inserts text
** at each "%%" line.  Also, any "P-a-r-s-e" identifer prefix (without the
** interstitial "-" characters) contained in this template is changed into
** the value of the %name directive from the grammar.  Otherwise, the content
** of this template is copied straight through into the generate parser
** source file.
**
** The following is the concatenation of all %include directives from the
** input grammar file:
*/
#include <stdio.h>
/************ Begin %include sections from the grammar ************************/
#line 48 "parse.y"

#include "sqliteInt.h"

/*
** Disable all error recovery processing in the parser push-down
** automaton.
*/
#define YYNOERRORRECOVERY 1

/*
** Make yytestcase() the same as testcase()
*/
#define yytestcase(X) testcase(X)

/*
** Indicate that sqlite3ParserFree() will never be called with a null
** pointer.
*/
#define YYPARSEFREENEVERNULL 1

/*
** Alternative datatype for the argument to the malloc() routine passed
** into sqlite3ParserAlloc().  The default is size_t.
*/
#define YYMALLOCARGTYPE  u64

/*
** An instance of this structure holds information about the
** LIMIT clause of a SELECT statement.
*/
struct LimitVal {
  Expr *pLimit;    /* The LIMIT expression.  NULL if there is no limit */
  Expr *pOffset;   /* The OFFSET expression.  NULL if there is none */
};

/*
** An instance of the following structure describes the event of a
** TRIGGER.  "a" is the event type, one of TK_UPDATE, TK_INSERT,
** TK_DELETE, or TK_INSTEAD.  If the event is of the form
**
**      UPDATE ON (a,b,c)
**
** Then the "b" IdList records the list "a,b,c".
*/
struct TrigEvent { int a; IdList * b; };

/*
** Disable lookaside memory allocation for objects that might be
** shared across database connections.
*/
static void disableLookaside(Parse *pParse){
  pParse->disableLookaside++;
  pParse->db->lookaside.bDisable++;
}

#line 402 "parse.y"

  /*
  ** For a compound SELECT statement, make sure p->pPrior->pNext==p for
  ** all elements in the list.  And make sure list length does not exceed
  ** SQLITE_LIMIT_COMPOUND_SELECT.
  */
  static void parserDoubleLinkSelect(Parse *pParse, Select *p){
    if( p->pPrior ){
      Select *pNext = 0, *pLoop;
      int mxSelect, cnt = 0;
      for(pLoop=p; pLoop; pNext=pLoop, pLoop=pLoop->pPrior, cnt++){
        pLoop->pNext = pNext;
        pLoop->selFlags |= SF_Compound;
      }
      if( (p->selFlags & SF_MultiValue)==0 && 
        (mxSelect = pParse->db->aLimit[SQLITE_LIMIT_COMPOUND_SELECT])>0 &&
        cnt>mxSelect
      ){
        sqlite3ErrorMsg(pParse, "too many terms in compound SELECT");
      }
    }
  }
#line 825 "parse.y"

  /* This is a utility routine used to set the ExprSpan.zStart and
  ** ExprSpan.zEnd values of pOut so that the span covers the complete
  ** range of text beginning with pStart and going to the end of pEnd.
  */
  static void spanSet(ExprSpan *pOut, Token *pStart, Token *pEnd){
    pOut->zStart = pStart->z;
    pOut->zEnd = &pEnd->z[pEnd->n];
  }

  /* Construct a new Expr object from a single identifier.  Use the
  ** new Expr to populate pOut.  Set the span of pOut to be the identifier
  ** that created the expression.
  */
  static void spanExpr(ExprSpan *pOut, Parse *pParse, int op, Token t){
    Expr *p = sqlite3DbMallocRawNN(pParse->db, sizeof(Expr)+t.n+1);
    if( p ){
      memset(p, 0, sizeof(Expr));
      p->op = (u8)op;
      p->flags = EP_Leaf;
      p->iAgg = -1;
      p->u.zToken = (char*)&p[1];
      memcpy(p->u.zToken, t.z, t.n);
      p->u.zToken[t.n] = 0;
      if( sqlite3Isquote(p->u.zToken[0]) ){
        if( p->u.zToken[0]=='"' ) p->flags |= EP_DblQuoted;
        sqlite3Dequote(p->u.zToken);
      }
#if SQLITE_MAX_EXPR_DEPTH>0
      p->nHeight = 1;
#endif  
    }
    pOut->pExpr = p;
    pOut->zStart = t.z;
    pOut->zEnd = &t.z[t.n];
  }
#line 941 "parse.y"

  /* This routine constructs a binary expression node out of two ExprSpan
  ** objects and uses the result to populate a new ExprSpan object.
  */
  static void spanBinaryExpr(
    Parse *pParse,      /* The parsing context.  Errors accumulate here */
    int op,             /* The binary operation */
    ExprSpan *pLeft,    /* The left operand, and output */
    ExprSpan *pRight    /* The right operand */
  ){
    pLeft->pExpr = sqlite3PExpr(pParse, op, pLeft->pExpr, pRight->pExpr, 0);
    pLeft->zEnd = pRight->zEnd;
  }

  /* If doNot is true, then add a TK_NOT Expr-node wrapper around the
  ** outside of *ppExpr.
  */
  static void exprNot(Parse *pParse, int doNot, ExprSpan *pSpan){
    if( doNot ){
      pSpan->pExpr = sqlite3PExpr(pParse, TK_NOT, pSpan->pExpr, 0, 0);
    }
  }
#line 1015 "parse.y"

  /* Construct an expression node for a unary postfix operator
  */
  static void spanUnaryPostfix(
    Parse *pParse,         /* Parsing context to record errors */
    int op,                /* The operator */
    ExprSpan *pOperand,    /* The operand, and output */
    Token *pPostOp         /* The operand token for setting the span */
  ){
    pOperand->pExpr = sqlite3PExpr(pParse, op, pOperand->pExpr, 0, 0);
    pOperand->zEnd = &pPostOp->z[pPostOp->n];
  }                           
#line 1032 "parse.y"

  /* A routine to convert a binary TK_IS or TK_ISNOT expression into a
  ** unary TK_ISNULL or TK_NOTNULL expression. */
  static void binaryToUnaryIfNull(Parse *pParse, Expr *pY, Expr *pA, int op){
    sqlite3 *db = pParse->db;
    if( pA && pY && pY->op==TK_NULL ){
      pA->op = (u8)op;
      sqlite3ExprDelete(db, pA->pRight);
      pA->pRight = 0;
    }
  }
#line 1060 "parse.y"

  /* Construct an expression node for a unary prefix operator
  */
  static void spanUnaryPrefix(
    ExprSpan *pOut,        /* Write the new expression node here */
    Parse *pParse,         /* Parsing context to record errors */
    int op,                /* The operator */
    ExprSpan *pOperand,    /* The operand */
    Token *pPreOp         /* The operand token for setting the span */
  ){
    pOut->zStart = pPreOp->z;
    pOut->pExpr = sqlite3PExpr(pParse, op, pOperand->pExpr, 0, 0);
    pOut->zEnd = pOperand->zEnd;
  }
#line 1272 "parse.y"

  /* Add a single new term to an ExprList that is used to store a
  ** list of identifiers.  Report an error if the ID list contains
  ** a COLLATE clause or an ASC or DESC keyword, except ignore the
  ** error while parsing a legacy schema.
  */
  static ExprList *parserAddExprIdListTerm(
    Parse *pParse,
    ExprList *pPrior,
    Token *pIdToken,
    int hasCollate,
    int sortOrder
  ){
    ExprList *p = sqlite3ExprListAppend(pParse, pPrior, 0);
    if( (hasCollate || sortOrder!=SQLITE_SO_UNDEFINED)
        && pParse->db->init.busy==0
    ){
      sqlite3ErrorMsg(pParse, "syntax error after column name \"%.*s\"",
                         pIdToken->n, pIdToken->z);
    }
    sqlite3ExprListSetName(pParse, p, pIdToken, 1);
    return p;
  }
#line 231 "parse.c"
/**************** End of %include directives **********************************/
/* These constants specify the various numeric values for terminal symbols
** in a format understandable to "makeheaders".  This section is blank unless
** "lemon" is run with the "-m" command-line option.
***************** Begin makeheaders token definitions *************************/
/**************** End makeheaders token definitions ***************************/

/* The next sections is a series of control #defines.
** various aspects of the generated parser.
**    YYCODETYPE         is the data type used to store the integer codes
**                       that represent terminal and non-terminal symbols.
**                       "unsigned char" is used if there are fewer than
**                       256 symbols.  Larger types otherwise.
**    YYNOCODE           is a number of type YYCODETYPE that is not used for
**                       any terminal or nonterminal symbol.
**    YYFALLBACK         If defined, this indicates that one or more tokens
**                       (also known as: "terminal symbols") have fall-back
**                       values which should be used if the original symbol
**                       would not parse.  This permits keywords to sometimes
**                       be used as identifiers, for example.
**    YYACTIONTYPE       is the data type used for "action codes" - numbers
**                       that indicate what to do in response to the next
**                       token.
**    sqlite3ParserTOKENTYPE     is the data type used for minor type for terminal
**                       symbols.  Background: A "minor type" is a semantic
**                       value associated with a terminal or non-terminal
**                       symbols.  For example, for an "ID" terminal symbol,
**                       the minor type might be the name of the identifier.
**                       Each non-terminal can have a different minor type.
**                       Terminal symbols all have the same minor type, though.
**                       This macros defines the minor type for terminal 
**                       symbols.
**    YYMINORTYPE        is the data type used for all minor types.
**                       This is typically a union of many types, one of
**                       which is sqlite3ParserTOKENTYPE.  The entry in the union
**                       for terminal symbols is called "yy0".
**    YYSTACKDEPTH       is the maximum depth of the parser's stack.  If
**                       zero the stack is dynamically sized using realloc()
**    sqlite3ParserARG_SDECL     A static variable declaration for the %extra_argument
**    sqlite3ParserARG_PDECL     A parameter declaration for the %extra_argument
**    sqlite3ParserARG_STORE     Code to store %extra_argument into yypParser
**    sqlite3ParserARG_FETCH     Code to extract %extra_argument from yypParser
**    YYERRORSYMBOL      is the code number of the error symbol.  If not
**                       defined, then do no error processing.
**    YYNSTATE           the combined number of states.
**    YYNRULE            the number of rules in the grammar
**    YY_MAX_SHIFT       Maximum value for shift actions
**    YY_MIN_SHIFTREDUCE Minimum value for shift-reduce actions
**    YY_MAX_SHIFTREDUCE Maximum value for shift-reduce actions
**    YY_MIN_REDUCE      Maximum value for reduce actions
**    YY_ERROR_ACTION    The yy_action[] code for syntax error
**    YY_ACCEPT_ACTION   The yy_action[] code for accept
**    YY_NO_ACTION       The yy_action[] code for no-op
*/
#ifndef INTERFACE
# define INTERFACE 1
#endif
/************* Begin control #defines *****************************************/
#define YYCODETYPE unsigned char
#define YYNOCODE 229
#define YYACTIONTYPE unsigned short int
#define YYWILDCARD 99
#define sqlite3ParserTOKENTYPE Token
typedef union {
  int yyinit;
  sqlite3ParserTOKENTYPE yy0;
  With* yy163;
  struct LimitVal yy196;
  struct {int value; int mask;} yy215;
  Select* yy219;
  ExprList* yy250;
  SrcList* yy259;
  Expr* yy362;
  ExprSpan yy382;
  int yy388;
  IdList* yy432;
} YYMINORTYPE;
#ifndef YYSTACKDEPTH
#define YYSTACKDEPTH 100
#endif
#define sqlite3ParserARG_SDECL Parse *pParse;
#define sqlite3ParserARG_PDECL ,Parse *pParse
#define sqlite3ParserARG_FETCH Parse *pParse = yypParser->pParse
#define sqlite3ParserARG_STORE yypParser->pParse = pParse
#define YYFALLBACK 1
#define YYNSTATE             326
#define YYNRULE              251
#define YY_MAX_SHIFT         325
#define YY_MIN_SHIFTREDUCE   486
#define YY_MAX_SHIFTREDUCE   736
#define YY_MIN_REDUCE        737
#define YY_MAX_REDUCE        987
#define YY_ERROR_ACTION      988
#define YY_ACCEPT_ACTION     989
#define YY_NO_ACTION         990
/************* End control #defines *******************************************/

/* Define the yytestcase() macro to be a no-op if is not already defined
** otherwise.
**
** Applications can choose to define yytestcase() in the %include section
** to a macro that can assist in verifying code coverage.  For production
** code the yytestcase() macro should be turned off.  But it is useful
** for testing.
*/
#ifndef yytestcase
# define yytestcase(X)
#endif


/* Next are the tables used to determine what action to take based on the
** current state and lookahead token.  These tables are used to implement
** functions that take a state number and lookahead value and return an
** action integer.  
**
** Suppose the action integer is N.  Then the action is determined as
** follows
**
**   0 <= N <= YY_MAX_SHIFT             Shift N.  That is, push the lookahead
**                                      token onto the stack and goto state N.
**
**   N between YY_MIN_SHIFTREDUCE       Shift to an arbitrary state then
**     and YY_MAX_SHIFTREDUCE           reduce by rule N-YY_MIN_SHIFTREDUCE.
**
**   N between YY_MIN_REDUCE            Reduce by rule N-YY_MIN_REDUCE
**     and YY_MAX_REDUCE
**
**   N == YY_ERROR_ACTION               A syntax error has occurred.
**
**   N == YY_ACCEPT_ACTION              The parser accepts its input.
**
**   N == YY_NO_ACTION                  No such action.  Denotes unused
**                                      slots in the yy_action[] table.
**
** The action table is constructed as a single large table named yy_action[].
** Given state S and lookahead X, the action is computed as either:
**
**    (A)   N = yy_action[ yy_shift_ofst[S] + X ]
**    (B)   N = yy_default[S]
**
** The (A) formula is preferred.  The B formula is used instead if:
**    (1)  The yy_shift_ofst[S]+X value is out of range, or
**    (2)  yy_lookahead[yy_shift_ofst[S]+X] is not equal to X, or
**    (3)  yy_shift_ofst[S] equal YY_SHIFT_USE_DFLT.
** (Implementation note: YY_SHIFT_USE_DFLT is chosen so that
** YY_SHIFT_USE_DFLT+X will be out of range for all possible lookaheads X.
** Hence only tests (1) and (2) need to be evaluated.)
**
** The formulas above are for computing the action when the lookahead is
** a terminal symbol.  If the lookahead is a non-terminal (as occurs after
** a reduce action) then the yy_reduce_ofst[] array is used in place of
** the yy_shift_ofst[] array and YY_REDUCE_USE_DFLT is used in place of
** YY_SHIFT_USE_DFLT.
**
** The following are the tables generated in this section:
**
**  yy_action[]        A single table containing all actions.
**  yy_lookahead[]     A table containing the lookahead for each entry in
**                     yy_action.  Used to detect hash collisions.
**  yy_shift_ofst[]    For each state, the offset into yy_action for
**                     shifting terminals.
**  yy_reduce_ofst[]   For each state, the offset into yy_action for
**                     shifting non-terminals after a reduce.
**  yy_default[]       Default action for each state.
**
*********** Begin parsing tables **********************************************/
#define YY_ACTTAB_COUNT (1201)
static const YYACTIONTYPE yy_action[] = {
 /*     0 */   241,    5,  732,  155,  155,  950,  950,   83,   84,   74,
 /*    10 */   650,  650,  662,  665,  654,  654,   81,   81,   82,   82,
 /*    20 */    82,   82,  703,   80,   80,   80,   80,   79,   79,   78,
 /*    30 */    78,   78,   77,  262,  733,  733,  241,   78,   78,   78,
 /*    40 */    77,  262,  262,   83,   84,   74,  650,  650,  662,  665,
 /*    50 */   654,  654,   81,   81,   82,   82,   82,   82,  264,   80,
 /*    60 */    80,   80,   80,   79,   79,   78,   78,   78,   77,  262,
 /*    70 */    79,   79,   78,   78,   78,   77,  262,  600,   76,   73,
 /*    80 */   140,  241,  513,  733,  733,  516,  601,   60,   83,   84,
 /*    90 */    74,  650,  650,  662,  665,  654,  654,   81,   81,   82,
 /*   100 */    82,   82,   82,  128,   80,   80,   80,   80,   79,   79,
 /*   110 */    78,   78,   78,   77,  262,  145,  599,  293,  287,  284,
 /*   120 */   283,   77,  262,  241,  184,  297,  199,  554,  554,  282,
 /*   130 */    83,   84,   74,  650,  650,  662,  665,  654,  654,   81,
 /*   140 */    81,   82,   82,   82,   82,   85,   80,   80,   80,   80,
 /*   150 */    79,   79,   78,   78,   78,   77,  262,  241,  552,  552,
 /*   160 */   521,   76,   73,  140,   83,   84,   74,  650,  650,  662,
 /*   170 */   665,  654,  654,   81,   81,   82,   82,   82,   82,  515,
 /*   180 */    80,   80,   80,   80,   79,   79,   78,   78,   78,   77,
 /*   190 */   262,  241,  565,  566,  514,  288,  288,  288,   83,   84,
 /*   200 */    74,  650,  650,  662,  665,  654,  654,   81,   81,   82,
 /*   210 */    82,   82,   82,  725,   80,   80,   80,   80,   79,   79,
 /*   220 */    78,   78,   78,   77,  262,  241,  203,  619,  641,  253,
 /*   230 */   253,  253,   83,   84,   74,  650,  650,  662,  665,  654,
 /*   240 */   654,   81,   81,   82,   82,   82,   82,  269,   80,   80,
 /*   250 */    80,   80,   79,   79,   78,   78,   78,   77,  262,  241,
 /*   260 */   102,  249,  627,  489,  490,  491,   83,   84,   74,  650,
 /*   270 */   650,  662,  665,  654,  654,   81,   81,   82,   82,   82,
 /*   280 */    82,  319,   80,   80,   80,   80,   79,   79,   78,   78,
 /*   290 */    78,   77,  262,  241,  989,  125,  125,    3,  626,  685,
 /*   300 */    83,   84,   74,  650,  650,  662,  665,  654,  654,   81,
 /*   310 */    81,   82,   82,   82,   82,  220,   80,   80,   80,   80,
 /*   320 */    79,   79,   78,   78,   78,   77,  262,  241,  307,  585,
 /*   330 */   114,  304,   99,  937,   83,   84,   74,  650,  650,  662,
 /*   340 */   665,  654,  654,   81,   81,   82,   82,   82,   82,  138,
 /*   350 */    80,   80,   80,   80,   79,   79,   78,   78,   78,   77,
 /*   360 */   262,  241,  244,  714,  714,  511,  191,  305,   83,   84,
 /*   370 */    74,  650,  650,  662,  665,  654,  654,   81,   81,   82,
 /*   380 */    82,   82,   82,  267,   80,   80,   80,   80,   79,   79,
 /*   390 */    78,   78,   78,   77,  262,   80,   80,   80,   80,   79,
 /*   400 */    79,   78,   78,   78,   77,  262,  191,  187,  139,  241,
 /*   410 */   203,  222,  715,  716,  511,   63,   83,   72,   74,  650,
 /*   420 */   650,  662,  665,  654,  654,   81,   81,   82,   82,   82,
 /*   430 */    82,  585,   80,   80,   80,   80,   79,   79,   78,   78,
 /*   440 */    78,   77,  262,  241,  640,  256,  634,  173,  172,   66,
 /*   450 */   628,   84,   74,  650,  650,  662,  665,  654,  654,   81,
 /*   460 */    81,   82,   82,   82,   82,  532,   80,   80,   80,   80,
 /*   470 */    79,   79,   78,   78,   78,   77,  262,  241,  633,  633,
 /*   480 */   635,  167,  302,  191,  585,  273,   74,  650,  650,  662,
 /*   490 */   665,  654,  654,   81,   81,   82,   82,   82,   82,  134,
 /*   500 */    80,   80,   80,   80,   79,   79,   78,   78,   78,   77,
 /*   510 */   262,   71,  318,    2,  930,  102,  651,  651,  663,  666,
 /*   520 */   150,   71,  318,    2,  174,   59,   71,  318,    2,  152,
 /*   530 */   137,   61,  318,    2,  571,  721,  325,  486,   68,   69,
 /*   540 */   714,  714,  185,  523,  123,   70,  263,  263,   68,   69,
 /*   550 */   570,  560,  504,   68,   69,   70,  263,  263,   68,   69,
 /*   560 */    70,  263,  263,  139,  317,   70,  263,  263,   82,   82,
 /*   570 */    82,   82,   75,   80,   80,   80,   80,   79,   79,   78,
 /*   580 */    78,   78,   77,  262,   19,  102,  585,  561,  561,  715,
 /*   590 */   716,  194,   87,  640,  655,  322,  321,  102,  300,  628,
 /*   600 */   259,  115,  320,  640,  243,  322,  321,   57,  640,  628,
 /*   610 */   322,  321,  526,  640,  628,  322,  321,  133,  132,  628,
 /*   620 */   252,  251,    9,    9,  278,  517,  517,  633,  633,  635,
 /*   630 */   636,   17,  626,  254,  714,  714,  248,  633,  633,  635,
 /*   640 */   636,   17,  633,  633,  635,  636,   17,  633,  633,  635,
 /*   650 */   636,   17,  714,  714,   82,   82,   82,   82,  196,   80,
 /*   660 */    80,   80,   80,   79,   79,   78,   78,   78,   77,  262,
 /*   670 */   714,  714,  640,   67,  634,   65,  549,  528,  628,  149,
 /*   680 */   148,  147,  626,  715,  716,  549,  160,  131,  211,  291,
 /*   690 */   206,  290,  151,  320,  275,  320,  245,  320,  696,  202,
 /*   700 */   191,  715,  716,  507,  242,  198,  633,  633,  635,  736,
 /*   710 */   622,  736,  320,   42,   42,   42,   42,   42,   42,  715,
 /*   720 */   716,  145,  171,  314,  287,  284,  283,  235,  234,  233,
 /*   730 */   163,  231,   42,   42,  499,  282,   20,  186,  271,  188,
 /*   740 */   191,  693,  697,  320,  184,  297,  246,  313,  298,  313,
 /*   750 */   303,  313,  312,  598,  295,  279,  102,  320,    1,  698,
 /*   760 */   545,  555,  626,   26,   26,  150,  313,  315,  320,  714,
 /*   770 */   714,  951,  703,  556,  714,  714,  531,   42,   42,  714,
 /*   780 */   714,  557,  569,  204,  320,  718,  320,  102,    9,    9,
 /*   790 */   530,  714,  714,   76,   73,  140,  320,  296,   76,   73,
 /*   800 */   140,  320,  250,  529,   42,   42,    9,    9,  701,    3,
 /*   810 */   320,  313,  294,  124,  169,  137,   42,   42,  715,  716,
 /*   820 */   175,    9,    9,  715,  716,  240,  261,  600,  715,  716,
 /*   830 */    41,   41,  561,  561,  718,  310,  601,  247,  260,  535,
 /*   840 */   715,  716,  155,  300,  176,  183,  176,  587,  301,  243,
 /*   850 */   308,  205,  586,  289,  202,   48,  265,  320,  975,  320,
 /*   860 */   576,  544,  624,  536,  320,  154,  320,  266,    8,    8,
 /*   870 */   154,  565,  566,  733,  320,  192,  320,   29,   29,   30,
 /*   880 */    30,  320,  223,  320,   31,   31,   32,   32,  320,  534,
 /*   890 */   533,  524,  257,  320,   22,   22,   23,   23,  320,  592,
 /*   900 */   320,   25,   25,   33,   33,  320,  170,  320,   34,   34,
 /*   910 */   320,  625,  320,   35,   35,  320,  692,  320,   10,   10,
 /*   920 */    36,   36,  733,  210,  320,   89,   89,   37,   37,  320,
 /*   930 */    38,   38,   27,   27,  209,   39,   39,   40,   40,  320,
 /*   940 */   524,  100,  320,  127,   28,   28,  320,  572,  320,  104,
 /*   950 */   104,  320,  181,  320,  583,  320,  695,  320,  695,  105,
 /*   960 */   105,  320,  106,  106,  320,  692,   46,   46,   90,   90,
 /*   970 */   320,   43,   43,   91,   91,   92,   92,   88,   88,  320,
 /*   980 */   632,  103,  103,  320,  101,  101,  320,  694,  320,  694,
 /*   990 */    96,   96,  320,  268,  320,  193,  154,  320,   59,   95,
 /*  1000 */    95,  320,  190,   93,   93,  637,   94,   94,   45,   45,
 /*  1010 */   965,  512,   47,   47,   44,   44,  506,   21,   21,  124,
 /*  1020 */   280,   24,   24,  146,  541,  542,  200,  230,  496,   59,
 /*  1030 */   495,  237,  509,  688,  563,   98,  146,   62,  594,  497,
 /*  1040 */   708,  154,  213,  236,  215,    7,  285,  614,  217,  135,
 /*  1050 */   690,  528,  689,  228,  637,  208,  270,  189,  316,  130,
 /*  1060 */   711,  110,  621,  219,  112,   57,  611,  272,  116,  157,
 /*  1070 */   126,  584,  684,  118,  119,  274,  277,  142,  121,  498,
 /*  1080 */   143,  238,  292,  548,  547,  546,  255,  526,  539,  258,
 /*  1090 */   239,  520,  306,   56,  519,  156,  177,  178,  207,  518,
 /*  1100 */   723,    6,  221,   64,  580,  538,  212,  670,  182,  581,
 /*  1110 */   214,   86,  579,  216,  311,  578,  218,   58,  562,  309,
 /*  1120 */   161,  503,   18,  323,  709,  165,  162,   97,  164,  226,
 /*  1130 */   224,  225,  227,  324,  493,  492,  487,  620,  179,  113,
 /*  1140 */    49,   50,  107,  108,   51,   52,  109,  117,  558,  141,
 /*  1150 */    11,  195,  501,  276,  197,  209,  144,  120,  281,  201,
 /*  1160 */    53,   12,   13,  286,  510,   54,  180,  537,  111,  639,
 /*  1170 */   638,  668,   14,  136,  564,   55,    4,  593,  299,  166,
 /*  1180 */   168,  153,  122,  588,   62,   59,   15,   16,  683,  669,
 /*  1190 */   667,  672,  671,  129,  158,  159,  967,  966,  229,  232,
 /*  1200 */   704,
};
static const YYCODETYPE yy_lookahead[] = {
 /*     0 */    16,   18,   22,   20,   20,  118,  119,   23,   24,   25,
 /*    10 */    26,   27,   28,   29,   30,   31,   32,   33,   34,   35,
 /*    20 */    36,   37,    1,   39,   40,   41,   42,   43,   44,   45,
 /*    30 */    46,   47,   48,   49,   51,   51,   16,   45,   46,   47,
 /*    40 */    48,   49,   49,   23,   24,   25,   26,   27,   28,   29,
 /*    50 */    30,   31,   32,   33,   34,   35,   36,   37,  103,   39,
 /*    60 */    40,   41,   42,   43,   44,   45,   46,   47,   48,   49,
 /*    70 */    43,   44,   45,   46,   47,   48,   49,   57,  218,  219,
 /*    80 */   220,   16,  170,  100,  100,  170,   66,   22,   23,   24,
 /*    90 */    25,   26,   27,   28,   29,   30,   31,   32,   33,   34,
 /*   100 */    35,   36,   37,  123,   39,   40,   41,   42,   43,   44,
 /*   110 */    45,   46,   47,   48,   49,  102,  173,   45,  105,  106,
 /*   120 */   107,   48,   49,   16,  118,  119,   19,  188,  189,  116,
 /*   130 */    23,   24,   25,   26,   27,   28,   29,   30,   31,   32,
 /*   140 */    33,   34,   35,   36,   37,   80,   39,   40,   41,   42,
 /*   150 */    43,   44,   45,   46,   47,   48,   49,   16,  188,  189,
 /*   160 */    19,  218,  219,  220,   23,   24,   25,   26,   27,   28,
 /*   170 */    29,   30,   31,   32,   33,   34,   35,   36,   37,  170,
 /*   180 */    39,   40,   41,   42,   43,   44,   45,   46,   47,   48,
 /*   190 */    49,   16,  120,  121,   19,  166,  167,  168,   23,   24,
 /*   200 */    25,   26,   27,   28,   29,   30,   31,   32,   33,   34,
 /*   210 */    35,   36,   37,  183,   39,   40,   41,   42,   43,   44,
 /*   220 */    45,   46,   47,   48,   49,   16,  150,  161,   19,  166,
 /*   230 */   167,  168,   23,   24,   25,   26,   27,   28,   29,   30,
 /*   240 */    31,   32,   33,   34,   35,   36,   37,  150,   39,   40,
 /*   250 */    41,   42,   43,   44,   45,   46,   47,   48,   49,   16,
 /*   260 */   194,  185,   19,    4,    5,    6,   23,   24,   25,   26,
 /*   270 */    27,   28,   29,   30,   31,   32,   33,   34,   35,   36,
 /*   280 */    37,  150,   39,   40,   41,   42,   43,   44,   45,   46,
 /*   290 */    47,   48,   49,   16,  142,  143,  144,  145,  150,  106,
 /*   300 */    23,   24,   25,   26,   27,   28,   29,   30,   31,   32,
 /*   310 */    33,   34,   35,   36,   37,  150,   39,   40,   41,   42,
 /*   320 */    43,   44,   45,   46,   47,   48,   49,   16,  150,   22,
 /*   330 */    18,   16,   18,  140,   23,   24,   25,   26,   27,   28,
 /*   340 */    29,   30,   31,   32,   33,   34,   35,   36,   37,  150,
 /*   350 */    39,   40,   41,   42,   43,   44,   45,   46,   47,   48,
 /*   360 */    49,   16,  214,   51,   52,   51,  150,   52,   23,   24,
 /*   370 */    25,   26,   27,   28,   29,   30,   31,   32,   33,   34,
 /*   380 */    35,   36,   37,  150,   39,   40,   41,   42,   43,   44,
 /*   390 */    45,   46,   47,   48,   49,   39,   40,   41,   42,   43,
 /*   400 */    44,   45,   46,   47,   48,   49,  150,  191,  101,   16,
 /*   410 */   150,  150,  100,  101,  100,  138,   23,   24,   25,   26,
 /*   420 */    27,   28,   29,   30,   31,   32,   33,   34,   35,   36,
 /*   430 */    37,  124,   39,   40,   41,   42,   43,   44,   45,   46,
 /*   440 */    47,   48,   49,   16,   98,  185,  100,  191,  215,  138,
 /*   450 */   104,   24,   25,   26,   27,   28,   29,   30,   31,   32,
 /*   460 */    33,   34,   35,   36,   37,  179,   39,   40,   41,   42,
 /*   470 */    43,   44,   45,   46,   47,   48,   49,   16,  132,  133,
 /*   480 */   134,   19,  161,  150,   22,  150,   25,   26,   27,   28,
 /*   490 */    29,   30,   31,   32,   33,   34,   35,   36,   37,   22,
 /*   500 */    39,   40,   41,   42,   43,   44,   45,   46,   47,   48,
 /*   510 */    49,   16,   17,   18,   19,  194,   26,   27,   28,   29,
 /*   520 */    26,   16,   17,   18,  191,   22,   16,   17,   18,  208,
 /*   530 */   209,   16,   17,   18,   45,  169,  146,  147,   43,   44,
 /*   540 */    51,   52,  152,  177,  154,   50,   51,   52,   43,   44,
 /*   550 */    45,  161,  161,   43,   44,   50,   51,   52,   43,   44,
 /*   560 */    50,   51,   52,  101,  161,   50,   51,   52,   34,   35,
 /*   570 */    36,   37,   38,   39,   40,   41,   42,   43,   44,   45,
 /*   580 */    46,   47,   48,   49,  194,  194,  124,  192,  193,  100,
 /*   590 */   101,  150,   18,   98,  104,  100,  101,  194,  203,  104,
 /*   600 */    16,   18,  150,   98,  110,  100,  101,  130,   98,  104,
 /*   610 */   100,  101,  109,   98,  104,  100,  101,   43,   44,  104,
 /*   620 */    43,   44,  170,  171,  155,   51,   52,  132,  133,  134,
 /*   630 */   135,  136,  150,   49,   51,   52,  184,  132,  133,  134,
 /*   640 */   135,  136,  132,  133,  134,  135,  136,  132,  133,  134,
 /*   650 */   135,  136,   51,   52,   34,   35,   36,   37,  150,   39,
 /*   660 */    40,   41,   42,   43,   44,   45,   46,   47,   48,   49,
 /*   670 */    51,   52,   98,  137,  100,  139,  177,  178,  104,  111,
 /*   680 */   112,  113,  150,  100,  101,  186,  102,  103,  104,  105,
 /*   690 */   106,  107,  108,  150,  225,  150,  214,  150,    9,  115,
 /*   700 */   150,  100,  101,  164,  165,  150,  132,  133,  134,  132,
 /*   710 */    84,  134,  150,  170,  171,  170,  171,  170,  171,  100,
 /*   720 */   101,  102,    2,  161,  105,  106,  107,    7,    8,    9,
 /*   730 */    10,   11,  170,  171,   14,  116,   18,  111,  112,  113,
 /*   740 */   150,  191,   53,  150,  118,  119,  214,  204,  205,  204,
 /*   750 */   205,  204,  205,  173,  161,   16,  194,  150,   18,   70,
 /*   760 */   150,   72,  150,  170,  171,   26,  204,  205,  150,   51,
 /*   770 */    52,    0,    1,   84,   51,   52,  179,  170,  171,   51,
 /*   780 */    52,  191,  207,  150,  150,   51,  150,  194,  170,  171,
 /*   790 */   179,   51,   52,  218,  219,  220,  150,  204,  218,  219,
 /*   800 */   220,  150,  184,  150,  170,  171,  170,  171,  144,  145,
 /*   810 */   150,  204,  205,  150,  208,  209,  170,  171,  100,  101,
 /*   820 */   184,  170,  171,  100,  101,  162,  214,   57,  100,  101,
 /*   830 */   170,  171,  192,  193,  100,  184,   66,  117,  204,   61,
 /*   840 */   100,  101,   20,  203,  181,  196,  183,  124,  150,  110,
 /*   850 */   204,  150,  124,   75,  115,  206,  216,  150,   19,  150,
 /*   860 */   210,   22,   19,   85,  150,   22,  150,   19,  170,  171,
 /*   870 */    22,  120,  121,   51,  150,   13,  150,  170,  171,  170,
 /*   880 */   171,  150,  222,  150,  170,  171,  170,  171,  150,  103,
 /*   890 */   104,   51,  114,  150,  170,  171,  170,  171,  150,  150,
 /*   900 */   150,  170,  171,  170,  171,  150,   18,  150,  170,  171,
 /*   910 */   150,  150,  150,  170,  171,  150,   51,  150,  170,  171,
 /*   920 */   170,  171,  100,  104,  150,  170,  171,  170,  171,  150,
 /*   930 */   170,  171,  170,  171,  115,  170,  171,  170,  171,  150,
 /*   940 */   100,   18,  150,   20,  170,  171,  150,  150,  150,  170,
 /*   950 */   171,  150,  207,  150,  150,  150,  132,  150,  134,  170,
 /*   960 */   171,  150,  170,  171,  150,  100,  170,  171,  170,  171,
 /*   970 */   150,  170,  171,  170,  171,  170,  171,  170,  171,  150,
 /*   980 */   150,  170,  171,  150,  170,  171,  150,  132,  150,  134,
 /*   990 */   170,  171,  150,   19,  150,   19,   22,  150,   22,  170,
 /*  1000 */   171,  150,  140,  170,  171,   51,  170,  171,  170,  171,
 /*  1010 */   122,  150,  170,  171,  170,  171,  150,  170,  171,  150,
 /*  1020 */    19,  170,  171,   22,    4,    5,   19,  158,  150,   22,
 /*  1030 */   150,  162,   19,   19,   19,   22,   22,   22,   19,  150,
 /*  1040 */   150,   22,  207,  148,  207,  195,  174,  198,  207,  182,
 /*  1050 */   173,  178,  173,  197,  100,  173,  211,  211,  224,  195,
 /*  1060 */   153,   23,  187,  211,   18,  130,  198,   15,  187,  122,
 /*  1070 */   217,  157,  198,  190,  190,  157,   15,  156,   18,  157,
 /*  1080 */   156,  175,  110,  172,  172,  172,   49,  109,  180,   73,
 /*  1090 */   175,  172,  125,  110,  174,  157,  226,  226,  172,  172,
 /*  1100 */   172,   18,  157,  137,  213,  180,  212,  221,  223,  213,
 /*  1110 */   212,  129,  213,  212,  126,  213,  212,  128,  202,  127,
 /*  1120 */    21,  160,   22,  159,   10,    3,  151,  176,  151,  199,
 /*  1130 */   201,  200,  198,  149,  149,  149,  149,  119,  176,  131,
 /*  1140 */    33,   33,  163,  163,   33,   33,  114,  123,   17,  110,
 /*  1150 */    18,  140,   17,   16,   13,  115,  108,   18,   78,   19,
 /*  1160 */    18,   18,   18,   78,   19,   18,   78,   54,   64,   19,
 /*  1170 */    19,   19,   18,  122,   19,   22,   18,   52,   22,   19,
 /*  1180 */    19,   60,   18,  124,   22,   22,   60,   60,   19,   19,
 /*  1190 */    19,    8,   19,   18,  122,  122,  122,  122,   19,   12,
 /*  1200 */     1,
};
#define YY_SHIFT_USE_DFLT (1201)
#define YY_SHIFT_COUNT    (325)
#define YY_SHIFT_MIN      (-113)
#define YY_SHIFT_MAX      (1199)
static const short yy_shift_ofst[] = {
 /*     0 */    21,  495,  510,  720,  510,  510,  510,  510,  -16,   20,
 /*    10 */    20,  510,  510,  510,  510,  510,  510,  510,  619,  626,
 /*    20 */  -113,   65,  107,  141,  175,  209,  243,  277,  311,  345,
 /*    30 */   345,  345,  345,  345,  345,  345,  345,  345,  345,  345,
 /*    40 */   345,  345,  345,  393,  345,  427,  461,  461,  505,  510,
 /*    50 */   510,  510,  510,  510,  510,  510,  510,  510,  510,  510,
 /*    60 */   510,  510,  510,  510,  510,  510,  510,  510,  510,  510,
 /*    70 */   510,  510,  510,  510,  515,  510,  510,  510,  510,  510,
 /*    80 */   510,  510,  510,  510,  510,  510,  510,  510,  534,  620,
 /*    90 */   620,  620,  620,  620,  356,   27,   -8,  739,  577,  577,
 /*   100 */   601,   73,    6,   -7, 1201, 1201, 1201,  584,  584,  312,
 /*   110 */   689,  689,  601,  601,  601,  601,  601,  601,  601,  601,
 /*   120 */   601,  601,  601,  193,  734,  771,  -45, 1201, 1201, 1201,
 /*   130 */  1201,  574,  346,  346,  583,   13,  489,  718,  723,  728,
 /*   140 */   740,  601,  601,  601,  601,  601,  601,  778,  778,  778,
 /*   150 */   601,  601,  462,  601,  601,  601,  -17,  601,  601,  601,
 /*   160 */   601,  601,  601,  601,  601,  601,  822,  822,  822,  307,
 /*   170 */    72,  259,  477,  315,  315,  503,  839,  770,  770,  494,
 /*   180 */   770,  315,  536,  -20,  751,  923, 1038, 1046, 1038,  935,
 /*   190 */  1052,  947, 1052,  935,  947, 1061,  947, 1061, 1060,  972,
 /*   200 */   972,  972, 1016, 1037, 1037, 1060,  972,  978,  972, 1016,
 /*   210 */   972,  972,  967,  983,  967,  983,  967,  983,  967,  983,
 /*   220 */   947, 1083,  947,  966,  982,  988,  989,  992,  935, 1099,
 /*   230 */  1100, 1114, 1114, 1122, 1122, 1122, 1122, 1201, 1201, 1201,
 /*   240 */  1201,  490,  314,  568,  843,  848,  974,  862,  976, 1001,
 /*   250 */  1007,  824,  855, 1013,  840,  865, 1014,  786, 1020,  819,
 /*   260 */  1015, 1019,  954,  888, 1018, 1008, 1107, 1108, 1111, 1112,
 /*   270 */  1032, 1024, 1131, 1132, 1039, 1011, 1135, 1137, 1141, 1040,
 /*   280 */  1048, 1139, 1080, 1142, 1143, 1140, 1144, 1085, 1145, 1113,
 /*   290 */  1147, 1088, 1104, 1150, 1151, 1152, 1153, 1154, 1155, 1158,
 /*   300 */  1156, 1051, 1160, 1161, 1125, 1121, 1164, 1059, 1162, 1126,
 /*   310 */  1163, 1127, 1169, 1162, 1170, 1171, 1183, 1173, 1175, 1072,
 /*   320 */  1073, 1074, 1075, 1179, 1187, 1199,
};
#define YY_REDUCE_USE_DFLT (-141)
#define YY_REDUCE_COUNT (240)
#define YY_REDUCE_MIN   (-140)
#define YY_REDUCE_MAX   (987)
static const short yy_reduce_ofst[] = {
 /*     0 */   152,  562,  593,  390,  543,  545,  547,  607,  575,  -57,
 /*    10 */   580,  452,  618,  636,  634,  646,  651,  660,  663,  640,
 /*    20 */   321, -140, -140, -140, -140, -140, -140, -140, -140, -140,
 /*    30 */  -140, -140, -140, -140, -140, -140, -140, -140, -140, -140,
 /*    40 */  -140, -140, -140, -140, -140, -140, -140, -140,  698,  707,
 /*    50 */   709,  714,  716,  724,  726,  731,  733,  738,  743,  748,
 /*    60 */   750,  755,  757,  760,  762,  765,  767,  774,  779,  789,
 /*    70 */   792,  796,  798,  801,  803,  805,  807,  811,  814,  820,
 /*    80 */   829,  833,  836,  838,  842,  844,  847,  851, -140, -140,
 /*    90 */  -140, -140, -140, -140, -140, -140, -140,  499,   29,   63,
 /*   100 */   869, -140,  395, -140, -140, -140, -140,  366,  366,  233,
 /*   110 */   -61,  -30,  148,  216,  482,  532,  256,  333,  550,  590,
 /*   120 */    76,  260,  612,  469,  539,  664,   66,  391,  606,  403,
 /*   130 */   649,  -88,  -85,    9,   97,   30,  131,  165,  178,  199,
 /*   140 */   261,  335,  441,  508,  555,  610,  633,  286,  597,  611,
 /*   150 */   653,  701,  650,  749,  761,  797,  745,  804,  830,  131,
 /*   160 */   861,  866,  878,  880,  889,  890,  835,  837,  841,  650,
 /*   170 */   850,  895,  849,  845,  846,  872,  867,  877,  879,  873,
 /*   180 */   882,  852,  834,  856,  864,  907,  875,  853,  881,  868,
 /*   190 */   883,  914,  884,  874,  918,  921,  922,  924,  906,  911,
 /*   200 */   912,  913,  908,  870,  871,  915,  919,  920,  926,  925,
 /*   210 */   927,  928,  891,  894,  896,  898,  899,  901,  902,  904,
 /*   220 */   938,  886,  945,  885,  916,  929,  931,  930,  934,  961,
 /*   230 */   964,  975,  977,  984,  985,  986,  987,  979,  951,  962,
 /*   240 */   980,
};
static const YYACTIONTYPE yy_default[] = {
 /*     0 */   956,  950,  950,  950,  930,  930,  930,  930,  824,  853,
 /*    10 */   853,  988,  988,  988,  988,  988,  988,  929,  988,  988,
 /*    20 */   828,  859,  988,  988,  988,  931,  932,  988,  988,  869,
 /*    30 */   868,  867,  866,  840,  864,  857,  861,  931,  925,  926,
 /*    40 */   924,  928,  932,  988,  860,  894,  909,  893,  988,  988,
 /*    50 */   988,  988,  988,  988,  988,  988,  988,  988,  988,  988,
 /*    60 */   988,  988,  988,  988,  988,  988,  988,  988,  988,  988,
 /*    70 */   988,  988,  988,  988,  988,  988,  988,  988,  988,  988,
 /*    80 */   988,  988,  988,  988,  988,  988,  988,  988,  903,  908,
 /*    90 */   915,  907,  904,  896,  895,  897,  898,  801,  988,  988,
 /*   100 */   988,  899,  988,  900,  912,  911,  910,  964,  963,  988,
 /*   110 */   988,  988,  988,  988,  988,  988,  988,  988,  988,  988,
 /*   120 */   988,  988,  988,  753,  759,  956,  950,  950,  828,  950,
 /*   130 */   819,  988,  988,  988,  988,  988,  988,  988,  988,  988,
 /*   140 */   988,  988,  988,  988,  988,  988,  988,  988,  988,  988,
 /*   150 */   988,  988,  988,  988,  988,  988,  824,  988,  988,  988,
 /*   160 */   988,  988,  988,  988,  988,  958,  824,  824,  824,  826,
 /*   170 */   818,  739,  863,  842,  842,  776,  978,  853,  853,  773,
 /*   180 */   853,  842,  927,  825,  818,  988,  804,  874,  804,  863,
 /*   190 */   810,  833,  810,  863,  833,  751,  833,  751,  938,  802,
 /*   200 */   802,  802,  791,  942,  942,  938,  802,  776,  802,  791,
 /*   210 */   802,  802,  846,  841,  846,  841,  846,  841,  846,  841,
 /*   220 */   833,  933,  833,  988,  858,  847,  856,  854,  863,  756,
 /*   230 */   794,  961,  961,  957,  957,  957,  957,  973,  778,  778,
 /*   240 */   973,  988,  968,  988,  988,  988,  988,  988,  988,  988,
 /*   250 */   988,  988,  988,  988,  988,  988,  988,  988,  988,  988,
 /*   260 */   988,  988,  988,  880,  988,  988,  988,  988,  988,  988,
 /*   270 */   988,  988,  988,  988,  988,  988,  988,  988,  988,  988,
 /*   280 */   988,  988,  988,  988,  988,  988,  988,  988,  988,  988,
 /*   290 */   988,  988,  988,  988,  988,  988,  988,  988,  988,  988,
 /*   300 */   982,  988,  988,  988,  988,  988,  988,  988,  855,  988,
 /*   310 */   848,  988,  988,  986,  988,  988,  988,  988,  988,  882,
 /*   320 */   988,  881,  885,  988,  745,  988,
};
/********** End of lemon-generated parsing tables *****************************/

/* The next table maps tokens (terminal symbols) into fallback tokens.  
** If a construct like the following:
** 
**      %fallback ID X Y Z.
**
** appears in the grammar, then ID becomes a fallback token for X, Y,
** and Z.  Whenever one of the tokens X, Y, or Z is input to the parser
** but it does not parse, the type of the token is changed to ID and
** the parse is retried before an error is thrown.
**
** This feature can be used, for example, to cause some keywords in a language
** to revert to identifiers if they keyword does not apply in the context where
** it appears.
*/
#ifdef YYFALLBACK
static const YYCODETYPE yyFallback[] = {
    0,  /*          $ => nothing */
    0,  /*       SEMI => nothing */
   51,  /*      BEGIN => ID */
    0,  /* TRANSACTION => nothing */
   51,  /*   DEFERRED => ID */
   51,  /*  IMMEDIATE => ID */
   51,  /*  EXCLUSIVE => ID */
    0,  /*     COMMIT => nothing */
   51,  /*        END => ID */
   51,  /*   ROLLBACK => ID */
   51,  /*  SAVEPOINT => ID */
   51,  /*    RELEASE => ID */
    0,  /*         TO => nothing */
    0,  /*      TABLE => nothing */
    0,  /*     CREATE => nothing */
   51,  /*         IF => ID */
    0,  /*        NOT => nothing */
    0,  /*     EXISTS => nothing */
    0,  /*         LP => nothing */
    0,  /*         RP => nothing */
    0,  /*         AS => nothing */
   51,  /*    WITHOUT => ID */
    0,  /*      COMMA => nothing */
    0,  /*         OR => nothing */
    0,  /*        AND => nothing */
    0,  /*         IS => nothing */
   51,  /*      MATCH => ID */
   51,  /*    LIKE_KW => ID */
    0,  /*    BETWEEN => nothing */
    0,  /*         IN => nothing */
    0,  /*     ISNULL => nothing */
    0,  /*    NOTNULL => nothing */
    0,  /*         NE => nothing */
    0,  /*         EQ => nothing */
    0,  /*         GT => nothing */
    0,  /*         LE => nothing */
    0,  /*         LT => nothing */
    0,  /*         GE => nothing */
    0,  /*     ESCAPE => nothing */
    0,  /*     BITAND => nothing */
    0,  /*      BITOR => nothing */
    0,  /*     LSHIFT => nothing */
    0,  /*     RSHIFT => nothing */
    0,  /*       PLUS => nothing */
    0,  /*      MINUS => nothing */
    0,  /*       STAR => nothing */
    0,  /*      SLASH => nothing */
    0,  /*        REM => nothing */
    0,  /*     CONCAT => nothing */
    0,  /*    COLLATE => nothing */
    0,  /*     BITNOT => nothing */
    0,  /*         ID => nothing */
    0,  /*    INDEXED => nothing */
   51,  /*      ABORT => ID */
   51,  /*     ACTION => ID */
   51,  /*      AFTER => ID */
   51,  /*    ANALYZE => ID */
   51,  /*        ASC => ID */
   51,  /*     ATTACH => ID */
   51,  /*     BEFORE => ID */
   51,  /*         BY => ID */
   51,  /*    CASCADE => ID */
   51,  /*       CAST => ID */
   51,  /*   COLUMNKW => ID */
   51,  /*   CONFLICT => ID */
   51,  /*   DATABASE => ID */
   51,  /*       DESC => ID */
   51,  /*     DETACH => ID */
   51,  /*       EACH => ID */
   51,  /*    EXPLAIN => ID */
   51,  /*       FAIL => ID */
   51,  /*        FOR => ID */
   51,  /*     IGNORE => ID */
   51,  /*  INITIALLY => ID */
   51,  /*    INSTEAD => ID */
   51,  /*         NO => ID */
   51,  /*       PLAN => ID */
   51,  /*      QUERY => ID */
   51,  /*        KEY => ID */
   51,  /*         OF => ID */
   51,  /*     OFFSET => ID */
   51,  /*     PRAGMA => ID */
   51,  /*      RAISE => ID */
   51,  /*  RECURSIVE => ID */
   51,  /*    REPLACE => ID */
   51,  /*   RESTRICT => ID */
   51,  /*        ROW => ID */
   51,  /*       TEMP => ID */
   51,  /*    TRIGGER => ID */
   51,  /*     VACUUM => ID */
   51,  /*       VIEW => ID */
   51,  /*    VIRTUAL => ID */
   51,  /*       WITH => ID */
   51,  /*     EXCEPT => ID */
   51,  /*  INTERSECT => ID */
   51,  /*      UNION => ID */
   51,  /*    REINDEX => ID */
   51,  /*     RENAME => ID */
   51,  /*   CTIME_KW => ID */
};
#endif /* YYFALLBACK */

/* The following structure represents a single element of the
** parser's stack.  Information stored includes:
**
**   +  The state number for the parser at this level of the stack.
**
**   +  The value of the token stored at this level of the stack.
**      (In other words, the "major" token.)
**
**   +  The semantic value stored at this level of the stack.  This is
**      the information used by the action routines in the grammar.
**      It is sometimes called the "minor" token.
**
** After the "shift" half of a SHIFTREDUCE action, the stateno field
** actually contains the reduce action for the second half of the
** SHIFTREDUCE.
*/
struct yyStackEntry {
  YYACTIONTYPE stateno;  /* The state-number, or reduce action in SHIFTREDUCE */
  YYCODETYPE major;      /* The major token value.  This is the code
                         ** number for the token at this stack level */
  YYMINORTYPE minor;     /* The user-supplied minor token value.  This
                         ** is the value of the token  */
};
typedef struct yyStackEntry yyStackEntry;

/* The state of the parser is completely contained in an instance of
** the following structure */
struct yyParser {
  yyStackEntry *yytos;          /* Pointer to top element of the stack */
#ifdef YYTRACKMAXSTACKDEPTH
  int yyhwm;                    /* High-water mark of the stack */
#endif
#ifndef YYNOERRORRECOVERY
  int yyerrcnt;                 /* Shifts left before out of the error */
#endif
  sqlite3ParserARG_SDECL                /* A place to hold %extra_argument */
#if YYSTACKDEPTH<=0
  int yystksz;                  /* Current side of the stack */
  yyStackEntry *yystack;        /* The parser's stack */
  yyStackEntry yystk0;          /* First stack entry */
#else
  yyStackEntry yystack[YYSTACKDEPTH];  /* The parser's stack */
#endif
};
typedef struct yyParser yyParser;

#ifndef NDEBUG
#include <stdio.h>
static FILE *yyTraceFILE = 0;
static char *yyTracePrompt = 0;
#endif /* NDEBUG */

#ifndef NDEBUG
/* 
** Turn parser tracing on by giving a stream to which to write the trace
** and a prompt to preface each trace message.  Tracing is turned off
** by making either argument NULL 
**
** Inputs:
** <ul>
** <li> A FILE* to which trace output should be written.
**      If NULL, then tracing is turned off.
** <li> A prefix string written at the beginning of every
**      line of trace output.  If NULL, then tracing is
**      turned off.
** </ul>
**
** Outputs:
** None.
*/
void sqlite3ParserTrace(FILE *TraceFILE, char *zTracePrompt){
  yyTraceFILE = TraceFILE;
  yyTracePrompt = zTracePrompt;
  if( yyTraceFILE==0 ) yyTracePrompt = 0;
  else if( yyTracePrompt==0 ) yyTraceFILE = 0;
}
#endif /* NDEBUG */

#ifndef NDEBUG
/* For tracing shifts, the names of all terminals and nonterminals
** are required.  The following table supplies these names */
static const char *const yyTokenName[] = { 
  "$",             "SEMI",          "BEGIN",         "TRANSACTION", 
  "DEFERRED",      "IMMEDIATE",     "EXCLUSIVE",     "COMMIT",      
  "END",           "ROLLBACK",      "SAVEPOINT",     "RELEASE",     
  "TO",            "TABLE",         "CREATE",        "IF",          
  "NOT",           "EXISTS",        "LP",            "RP",          
  "AS",            "WITHOUT",       "COMMA",         "OR",          
  "AND",           "IS",            "MATCH",         "LIKE_KW",     
  "BETWEEN",       "IN",            "ISNULL",        "NOTNULL",     
  "NE",            "EQ",            "GT",            "LE",          
  "LT",            "GE",            "ESCAPE",        "BITAND",      
  "BITOR",         "LSHIFT",        "RSHIFT",        "PLUS",        
  "MINUS",         "STAR",          "SLASH",         "REM",         
  "CONCAT",        "COLLATE",       "BITNOT",        "ID",          
  "INDEXED",       "ABORT",         "ACTION",        "AFTER",       
  "ANALYZE",       "ASC",           "ATTACH",        "BEFORE",      
  "BY",            "CASCADE",       "CAST",          "COLUMNKW",    
  "CONFLICT",      "DATABASE",      "DESC",          "DETACH",      
  "EACH",          "EXPLAIN",       "FAIL",          "FOR",         
  "IGNORE",        "INITIALLY",     "INSTEAD",       "NO",          
  "PLAN",          "QUERY",         "KEY",           "OF",          
  "OFFSET",        "PRAGMA",        "RAISE",         "RECURSIVE",   
  "REPLACE",       "RESTRICT",      "ROW",           "TEMP",        
  "TRIGGER",       "VACUUM",        "VIEW",          "VIRTUAL",     
  "WITH",          "EXCEPT",        "INTERSECT",     "UNION",       
  "REINDEX",       "RENAME",        "CTIME_KW",      "ANY",         
  "STRING",        "JOIN_KW",       "CONSTRAINT",    "DEFAULT",     
  "NULL",          "PRIMARY",       "UNIQUE",        "CHECK",       
  "REFERENCES",    "AUTOINCR",      "ON",            "INSERT",      
  "DELETE",        "UPDATE",        "SET",           "DEFERRABLE",  
  "FOREIGN",       "DROP",          "SELECT",        "VALUES",      
  "DISTINCT",      "ALL",           "DOT",           "FROM",        
  "JOIN",          "USING",         "ORDER",         "GROUP",       
  "HAVING",        "LIMIT",         "WHERE",         "INTO",        
  "FLOAT",         "BLOB",          "INTEGER",       "VARIABLE",    
  "CASE",          "WHEN",          "THEN",          "ELSE",        
  "INDEX",         "error",         "input",         "cmdlist",     
  "ecmd",          "explain",       "cmdx",          "cmd",         
  "transtype",     "trans_opt",     "nm",            "savepoint_opt",
  "create_table",  "create_table_args",  "createkw",      "temp",        
  "ifnotexists",   "dbnm",          "columnlist",    "conslist_opt",
  "table_options",  "select",        "columnname",    "carglist",    
  "typetoken",     "typename",      "signed",        "plus_num",    
  "minus_num",     "ccons",         "term",          "expr",        
  "onconf",        "sortorder",     "autoinc",       "eidlist_opt", 
  "refargs",       "defer_subclause",  "refarg",        "refact",      
  "init_deferred_pred_opt",  "conslist",      "tconscomma",    "tcons",       
  "sortlist",      "eidlist",       "defer_subclause_opt",  "orconf",      
  "resolvetype",   "raisetype",     "ifexists",      "fullname",    
  "selectnowith",  "oneselect",     "with",          "distinct",    
  "selcollist",    "from",          "where_opt",     "groupby_opt", 
  "having_opt",    "orderby_opt",   "limit_opt",     "values",      
  "nexprlist",     "exprlist",      "sclp",          "as",          
  "seltablist",    "stl_prefix",    "joinop",        "indexed_opt", 
  "on_opt",        "using_opt",     "idlist",        "setlist",     
  "insert_cmd",    "idlist_opt",    "likeop",        "between_op",  
  "in_op",         "paren_exprlist",  "case_operand",  "case_exprlist",
  "case_else",     "uniqueflag",    "collate",       "wqlist",      
};
#endif /* NDEBUG */

#ifndef NDEBUG
/* For tracing reduce actions, the names of all rules are required.
*/
static const char *const yyRuleName[] = {
 /*   0 */ "cmdx ::= cmd",
 /*   1 */ "cmd ::= BEGIN transtype trans_opt",
 /*   2 */ "transtype ::=",
 /*   3 */ "transtype ::= DEFERRED",
 /*   4 */ "transtype ::= IMMEDIATE",
 /*   5 */ "transtype ::= EXCLUSIVE",
 /*   6 */ "cmd ::= COMMIT trans_opt",
 /*   7 */ "cmd ::= END trans_opt",
 /*   8 */ "cmd ::= ROLLBACK trans_opt",
 /*   9 */ "cmd ::= SAVEPOINT nm",
 /*  10 */ "cmd ::= RELEASE savepoint_opt nm",
 /*  11 */ "cmd ::= ROLLBACK trans_opt TO savepoint_opt nm",
 /*  12 */ "create_table ::= createkw temp TABLE ifnotexists nm dbnm",
 /*  13 */ "createkw ::= CREATE",
 /*  14 */ "ifnotexists ::=",
 /*  15 */ "ifnotexists ::= IF NOT EXISTS",
 /*  16 */ "temp ::=",
 /*  17 */ "create_table_args ::= LP columnlist conslist_opt RP table_options",
 /*  18 */ "create_table_args ::= AS select",
 /*  19 */ "table_options ::=",
 /*  20 */ "table_options ::= WITHOUT nm",
 /*  21 */ "columnname ::= nm typetoken",
 /*  22 */ "typetoken ::=",
 /*  23 */ "typetoken ::= typename LP signed RP",
 /*  24 */ "typetoken ::= typename LP signed COMMA signed RP",
 /*  25 */ "typename ::= typename ID|STRING",
 /*  26 */ "ccons ::= CONSTRAINT nm",
 /*  27 */ "ccons ::= DEFAULT term",
 /*  28 */ "ccons ::= DEFAULT LP expr RP",
 /*  29 */ "ccons ::= DEFAULT PLUS term",
 /*  30 */ "ccons ::= DEFAULT MINUS term",
 /*  31 */ "ccons ::= DEFAULT ID|INDEXED",
 /*  32 */ "ccons ::= NOT NULL onconf",
 /*  33 */ "ccons ::= PRIMARY KEY sortorder onconf autoinc",
 /*  34 */ "ccons ::= UNIQUE onconf",
 /*  35 */ "ccons ::= CHECK LP expr RP",
 /*  36 */ "ccons ::= REFERENCES nm eidlist_opt refargs",
 /*  37 */ "ccons ::= defer_subclause",
 /*  38 */ "ccons ::= COLLATE ID|STRING",
 /*  39 */ "autoinc ::=",
 /*  40 */ "autoinc ::= AUTOINCR",
 /*  41 */ "refargs ::=",
 /*  42 */ "refargs ::= refargs refarg",
 /*  43 */ "refarg ::= MATCH nm",
 /*  44 */ "refarg ::= ON INSERT refact",
 /*  45 */ "refarg ::= ON DELETE refact",
 /*  46 */ "refarg ::= ON UPDATE refact",
 /*  47 */ "refact ::= SET NULL",
 /*  48 */ "refact ::= SET DEFAULT",
 /*  49 */ "refact ::= CASCADE",
 /*  50 */ "refact ::= RESTRICT",
 /*  51 */ "refact ::= NO ACTION",
 /*  52 */ "defer_subclause ::= NOT DEFERRABLE init_deferred_pred_opt",
 /*  53 */ "defer_subclause ::= DEFERRABLE init_deferred_pred_opt",
 /*  54 */ "init_deferred_pred_opt ::=",
 /*  55 */ "init_deferred_pred_opt ::= INITIALLY DEFERRED",
 /*  56 */ "init_deferred_pred_opt ::= INITIALLY IMMEDIATE",
 /*  57 */ "conslist_opt ::=",
 /*  58 */ "tconscomma ::= COMMA",
 /*  59 */ "tcons ::= CONSTRAINT nm",
 /*  60 */ "tcons ::= PRIMARY KEY LP sortlist autoinc RP onconf",
 /*  61 */ "tcons ::= UNIQUE LP sortlist RP onconf",
 /*  62 */ "tcons ::= CHECK LP expr RP onconf",
 /*  63 */ "tcons ::= FOREIGN KEY LP eidlist RP REFERENCES nm eidlist_opt refargs defer_subclause_opt",
 /*  64 */ "defer_subclause_opt ::=",
 /*  65 */ "onconf ::=",
 /*  66 */ "onconf ::= ON CONFLICT resolvetype",
 /*  67 */ "orconf ::=",
 /*  68 */ "orconf ::= OR resolvetype",
 /*  69 */ "resolvetype ::= IGNORE",
 /*  70 */ "resolvetype ::= REPLACE",
 /*  71 */ "cmd ::= DROP TABLE ifexists fullname",
 /*  72 */ "ifexists ::= IF EXISTS",
 /*  73 */ "ifexists ::=",
 /*  74 */ "cmd ::= select",
 /*  75 */ "select ::= with selectnowith",
 /*  76 */ "oneselect ::= SELECT distinct selcollist from where_opt groupby_opt having_opt orderby_opt limit_opt",
 /*  77 */ "values ::= VALUES LP nexprlist RP",
 /*  78 */ "values ::= values COMMA LP exprlist RP",
 /*  79 */ "distinct ::= DISTINCT",
 /*  80 */ "distinct ::= ALL",
 /*  81 */ "distinct ::=",
 /*  82 */ "sclp ::=",
 /*  83 */ "selcollist ::= sclp expr as",
 /*  84 */ "selcollist ::= sclp STAR",
 /*  85 */ "selcollist ::= sclp nm DOT STAR",
 /*  86 */ "as ::= AS nm",
 /*  87 */ "as ::=",
 /*  88 */ "from ::=",
 /*  89 */ "from ::= FROM seltablist",
 /*  90 */ "stl_prefix ::= seltablist joinop",
 /*  91 */ "stl_prefix ::=",
 /*  92 */ "seltablist ::= stl_prefix nm dbnm as indexed_opt on_opt using_opt",
 /*  93 */ "seltablist ::= stl_prefix nm dbnm LP exprlist RP as on_opt using_opt",
 /*  94 */ "seltablist ::= stl_prefix LP select RP as on_opt using_opt",
 /*  95 */ "seltablist ::= stl_prefix LP seltablist RP as on_opt using_opt",
 /*  96 */ "dbnm ::=",
 /*  97 */ "dbnm ::= DOT nm",
 /*  98 */ "fullname ::= nm dbnm",
 /*  99 */ "joinop ::= COMMA|JOIN",
 /* 100 */ "joinop ::= JOIN_KW JOIN",
 /* 101 */ "joinop ::= JOIN_KW nm JOIN",
 /* 102 */ "joinop ::= JOIN_KW nm nm JOIN",
 /* 103 */ "on_opt ::= ON expr",
 /* 104 */ "on_opt ::=",
 /* 105 */ "indexed_opt ::=",
 /* 106 */ "indexed_opt ::= INDEXED BY nm",
 /* 107 */ "indexed_opt ::= NOT INDEXED",
 /* 108 */ "using_opt ::= USING LP idlist RP",
 /* 109 */ "using_opt ::=",
 /* 110 */ "orderby_opt ::=",
 /* 111 */ "orderby_opt ::= ORDER BY sortlist",
 /* 112 */ "sortlist ::= sortlist COMMA expr sortorder",
 /* 113 */ "sortlist ::= expr sortorder",
 /* 114 */ "sortorder ::= ASC",
 /* 115 */ "sortorder ::= DESC",
 /* 116 */ "sortorder ::=",
 /* 117 */ "groupby_opt ::=",
 /* 118 */ "groupby_opt ::= GROUP BY nexprlist",
 /* 119 */ "having_opt ::=",
 /* 120 */ "having_opt ::= HAVING expr",
 /* 121 */ "limit_opt ::=",
 /* 122 */ "limit_opt ::= LIMIT expr",
 /* 123 */ "limit_opt ::= LIMIT expr OFFSET expr",
 /* 124 */ "limit_opt ::= LIMIT expr COMMA expr",
 /* 125 */ "cmd ::= with DELETE FROM fullname indexed_opt where_opt",
 /* 126 */ "where_opt ::=",
 /* 127 */ "where_opt ::= WHERE expr",
 /* 128 */ "cmd ::= with UPDATE orconf fullname indexed_opt SET setlist where_opt",
 /* 129 */ "setlist ::= setlist COMMA nm EQ expr",
 /* 130 */ "setlist ::= setlist COMMA LP idlist RP EQ expr",
 /* 131 */ "setlist ::= nm EQ expr",
 /* 132 */ "setlist ::= LP idlist RP EQ expr",
 /* 133 */ "cmd ::= with insert_cmd INTO fullname idlist_opt select",
 /* 134 */ "cmd ::= with insert_cmd INTO fullname idlist_opt DEFAULT VALUES",
 /* 135 */ "insert_cmd ::= INSERT orconf",
 /* 136 */ "insert_cmd ::= REPLACE",
 /* 137 */ "idlist_opt ::=",
 /* 138 */ "idlist_opt ::= LP idlist RP",
 /* 139 */ "idlist ::= idlist COMMA nm",
 /* 140 */ "idlist ::= nm",
 /* 141 */ "expr ::= LP expr RP",
 /* 142 */ "term ::= NULL",
 /* 143 */ "expr ::= ID|INDEXED",
 /* 144 */ "expr ::= JOIN_KW",
 /* 145 */ "expr ::= nm DOT nm",
 /* 146 */ "expr ::= nm DOT nm DOT nm",
 /* 147 */ "term ::= FLOAT|BLOB",
 /* 148 */ "term ::= STRING",
 /* 149 */ "term ::= INTEGER",
 /* 150 */ "expr ::= VARIABLE",
 /* 151 */ "expr ::= expr COLLATE ID|STRING",
 /* 152 */ "expr ::= ID|INDEXED LP distinct exprlist RP",
 /* 153 */ "expr ::= ID|INDEXED LP STAR RP",
 /* 154 */ "term ::= CTIME_KW",
 /* 155 */ "expr ::= LP nexprlist COMMA expr RP",
 /* 156 */ "expr ::= expr AND expr",
 /* 157 */ "expr ::= expr OR expr",
 /* 158 */ "expr ::= expr LT|GT|GE|LE expr",
 /* 159 */ "expr ::= expr EQ|NE expr",
 /* 160 */ "expr ::= expr BITAND|BITOR|LSHIFT|RSHIFT expr",
 /* 161 */ "expr ::= expr PLUS|MINUS expr",
 /* 162 */ "expr ::= expr STAR|SLASH|REM expr",
 /* 163 */ "expr ::= expr CONCAT expr",
 /* 164 */ "likeop ::= LIKE_KW|MATCH",
 /* 165 */ "likeop ::= NOT LIKE_KW|MATCH",
 /* 166 */ "expr ::= expr likeop expr",
 /* 167 */ "expr ::= expr likeop expr ESCAPE expr",
 /* 168 */ "expr ::= expr ISNULL|NOTNULL",
 /* 169 */ "expr ::= expr NOT NULL",
 /* 170 */ "expr ::= expr IS expr",
 /* 171 */ "expr ::= expr IS NOT expr",
 /* 172 */ "expr ::= NOT expr",
 /* 173 */ "expr ::= BITNOT expr",
 /* 174 */ "expr ::= MINUS expr",
 /* 175 */ "expr ::= PLUS expr",
 /* 176 */ "between_op ::= BETWEEN",
 /* 177 */ "between_op ::= NOT BETWEEN",
 /* 178 */ "expr ::= expr between_op expr AND expr",
 /* 179 */ "in_op ::= IN",
 /* 180 */ "in_op ::= NOT IN",
 /* 181 */ "expr ::= expr in_op LP exprlist RP",
 /* 182 */ "expr ::= LP select RP",
 /* 183 */ "expr ::= expr in_op LP select RP",
 /* 184 */ "expr ::= expr in_op nm dbnm paren_exprlist",
 /* 185 */ "expr ::= EXISTS LP select RP",
 /* 186 */ "expr ::= CASE case_operand case_exprlist case_else END",
 /* 187 */ "case_exprlist ::= case_exprlist WHEN expr THEN expr",
 /* 188 */ "case_exprlist ::= WHEN expr THEN expr",
 /* 189 */ "case_else ::= ELSE expr",
 /* 190 */ "case_else ::=",
 /* 191 */ "case_operand ::= expr",
 /* 192 */ "case_operand ::=",
 /* 193 */ "exprlist ::=",
 /* 194 */ "nexprlist ::= nexprlist COMMA expr",
 /* 195 */ "nexprlist ::= expr",
 /* 196 */ "paren_exprlist ::=",
 /* 197 */ "paren_exprlist ::= LP exprlist RP",
 /* 198 */ "cmd ::= createkw uniqueflag INDEX ifnotexists nm dbnm ON nm LP sortlist RP where_opt",
 /* 199 */ "uniqueflag ::= UNIQUE",
 /* 200 */ "uniqueflag ::=",
 /* 201 */ "eidlist_opt ::=",
 /* 202 */ "eidlist_opt ::= LP eidlist RP",
 /* 203 */ "eidlist ::= eidlist COMMA nm collate sortorder",
 /* 204 */ "eidlist ::= nm collate sortorder",
 /* 205 */ "collate ::=",
 /* 206 */ "collate ::= COLLATE ID|STRING",
 /* 207 */ "cmd ::= DROP INDEX ifexists fullname",
 /* 208 */ "plus_num ::= PLUS INTEGER|FLOAT",
 /* 209 */ "minus_num ::= MINUS INTEGER|FLOAT",
 /* 210 */ "raisetype ::= ROLLBACK",
 /* 211 */ "raisetype ::= ABORT",
 /* 212 */ "raisetype ::= FAIL",
 /* 213 */ "with ::=",
 /* 214 */ "input ::= cmdlist",
 /* 215 */ "cmdlist ::= cmdlist ecmd",
 /* 216 */ "cmdlist ::= ecmd",
 /* 217 */ "ecmd ::= SEMI",
 /* 218 */ "ecmd ::= explain cmdx SEMI",
 /* 219 */ "explain ::=",
 /* 220 */ "trans_opt ::=",
 /* 221 */ "trans_opt ::= TRANSACTION",
 /* 222 */ "trans_opt ::= TRANSACTION nm",
 /* 223 */ "savepoint_opt ::= SAVEPOINT",
 /* 224 */ "savepoint_opt ::=",
 /* 225 */ "cmd ::= create_table create_table_args",
 /* 226 */ "columnlist ::= columnlist COMMA columnname carglist",
 /* 227 */ "columnlist ::= columnname carglist",
 /* 228 */ "nm ::= ID|INDEXED",
 /* 229 */ "nm ::= STRING",
 /* 230 */ "nm ::= JOIN_KW",
 /* 231 */ "typetoken ::= typename",
 /* 232 */ "typename ::= ID|STRING",
 /* 233 */ "signed ::= plus_num",
 /* 234 */ "signed ::= minus_num",
 /* 235 */ "carglist ::= carglist ccons",
 /* 236 */ "carglist ::=",
 /* 237 */ "ccons ::= NULL onconf",
 /* 238 */ "conslist_opt ::= COMMA conslist",
 /* 239 */ "conslist ::= conslist tconscomma tcons",
 /* 240 */ "conslist ::= tcons",
 /* 241 */ "tconscomma ::=",
 /* 242 */ "defer_subclause_opt ::= defer_subclause",
 /* 243 */ "resolvetype ::= raisetype",
 /* 244 */ "selectnowith ::= oneselect",
 /* 245 */ "oneselect ::= values",
 /* 246 */ "sclp ::= selcollist COMMA",
 /* 247 */ "as ::= ID|STRING",
 /* 248 */ "expr ::= term",
 /* 249 */ "exprlist ::= nexprlist",
 /* 250 */ "plus_num ::= INTEGER|FLOAT",
};
#endif /* NDEBUG */


#if YYSTACKDEPTH<=0
/*
** Try to increase the size of the parser stack.  Return the number
** of errors.  Return 0 on success.
*/
static int yyGrowStack(yyParser *p){
  int newSize;
  int idx;
  yyStackEntry *pNew;

  newSize = p->yystksz*2 + 100;
  idx = p->yytos ? (int)(p->yytos - p->yystack) : 0;
  if( p->yystack==&p->yystk0 ){
    pNew = malloc(newSize*sizeof(pNew[0]));
    if( pNew ) pNew[0] = p->yystk0;
  }else{
    pNew = realloc(p->yystack, newSize*sizeof(pNew[0]));
  }
  if( pNew ){
    p->yystack = pNew;
    p->yytos = &p->yystack[idx];
#ifndef NDEBUG
    if( yyTraceFILE ){
      fprintf(yyTraceFILE,"%sStack grows from %d to %d entries.\n",
              yyTracePrompt, p->yystksz, newSize);
    }
#endif
    p->yystksz = newSize;
  }
  return pNew==0; 
}
#endif

/* Datatype of the argument to the memory allocated passed as the
** second argument to sqlite3ParserAlloc() below.  This can be changed by
** putting an appropriate #define in the %include section of the input
** grammar.
*/
#ifndef YYMALLOCARGTYPE
# define YYMALLOCARGTYPE size_t
#endif

/* 
** This function allocates a new parser.
** The only argument is a pointer to a function which works like
** malloc.
**
** Inputs:
** A pointer to the function used to allocate memory.
**
** Outputs:
** A pointer to a parser.  This pointer is used in subsequent calls
** to sqlite3Parser and sqlite3ParserFree.
*/
void *sqlite3ParserAlloc(void *(*mallocProc)(YYMALLOCARGTYPE)){
  yyParser *pParser;
  pParser = (yyParser*)(*mallocProc)( (YYMALLOCARGTYPE)sizeof(yyParser) );
  if( pParser ){
#ifdef YYTRACKMAXSTACKDEPTH
    pParser->yyhwm = 0;
#endif
#if YYSTACKDEPTH<=0
    pParser->yytos = NULL;
    pParser->yystack = NULL;
    pParser->yystksz = 0;
    if( yyGrowStack(pParser) ){
      pParser->yystack = &pParser->yystk0;
      pParser->yystksz = 1;
    }
#endif
#ifndef YYNOERRORRECOVERY
    pParser->yyerrcnt = -1;
#endif
    pParser->yytos = pParser->yystack;
    pParser->yystack[0].stateno = 0;
    pParser->yystack[0].major = 0;
  }
  return pParser;
}

/* The following function deletes the "minor type" or semantic value
** associated with a symbol.  The symbol can be either a terminal
** or nonterminal. "yymajor" is the symbol code, and "yypminor" is
** a pointer to the value to be deleted.  The code used to do the 
** deletions is derived from the %destructor and/or %token_destructor
** directives of the input grammar.
*/
static void yy_destructor(
  yyParser *yypParser,    /* The parser */
  YYCODETYPE yymajor,     /* Type code for object to destroy */
  YYMINORTYPE *yypminor   /* The object to be destroyed */
){
  sqlite3ParserARG_FETCH;
  switch( yymajor ){
    /* Here is inserted the actions which take place when a
    ** terminal or non-terminal is destroyed.  This can happen
    ** when the symbol is popped from the stack during a
    ** reduce or during error processing or when a parser is 
    ** being destroyed before it is finished parsing.
    **
    ** Note: during a reduce, the only symbols destroyed are those
    ** which appear on the RHS of the rule, but which are *not* used
    ** inside the C code.
    */
/********* Begin destructor definitions ***************************************/
    case 161: /* select */
    case 192: /* selectnowith */
    case 193: /* oneselect */
    case 203: /* values */
{
#line 396 "parse.y"
sqlite3SelectDelete(pParse->db, (yypminor->yy219));
#line 1383 "parse.c"
}
      break;
    case 170: /* term */
    case 171: /* expr */
{
#line 823 "parse.y"
sqlite3ExprDelete(pParse->db, (yypminor->yy382).pExpr);
#line 1391 "parse.c"
}
      break;
    case 175: /* eidlist_opt */
    case 184: /* sortlist */
    case 185: /* eidlist */
    case 196: /* selcollist */
    case 199: /* groupby_opt */
    case 201: /* orderby_opt */
    case 204: /* nexprlist */
    case 205: /* exprlist */
    case 206: /* sclp */
    case 215: /* setlist */
    case 221: /* paren_exprlist */
    case 223: /* case_exprlist */
{
#line 1270 "parse.y"
sqlite3ExprListDelete(pParse->db, (yypminor->yy250));
#line 1409 "parse.c"
}
      break;
    case 191: /* fullname */
    case 197: /* from */
    case 208: /* seltablist */
    case 209: /* stl_prefix */
{
#line 628 "parse.y"
sqlite3SrcListDelete(pParse->db, (yypminor->yy259));
#line 1419 "parse.c"
}
      break;
    case 194: /* with */
    case 227: /* wqlist */
{
#line 1547 "parse.y"
sqlite3WithDelete(pParse->db, (yypminor->yy163));
#line 1427 "parse.c"
}
      break;
    case 198: /* where_opt */
    case 200: /* having_opt */
    case 212: /* on_opt */
    case 222: /* case_operand */
    case 224: /* case_else */
{
#line 744 "parse.y"
sqlite3ExprDelete(pParse->db, (yypminor->yy362));
#line 1438 "parse.c"
}
      break;
    case 213: /* using_opt */
    case 214: /* idlist */
    case 217: /* idlist_opt */
{
#line 662 "parse.y"
sqlite3IdListDelete(pParse->db, (yypminor->yy432));
#line 1447 "parse.c"
}
      break;
/********* End destructor definitions *****************************************/
    default:  break;   /* If no destructor action specified: do nothing */
  }
}

/*
** Pop the parser's stack once.
**
** If there is a destructor routine associated with the token which
** is popped from the stack, then call it.
*/
static void yy_pop_parser_stack(yyParser *pParser){
  yyStackEntry *yytos;
  assert( pParser->yytos!=0 );
  assert( pParser->yytos > pParser->yystack );
  yytos = pParser->yytos--;
#ifndef NDEBUG
  if( yyTraceFILE ){
    fprintf(yyTraceFILE,"%sPopping %s\n",
      yyTracePrompt,
      yyTokenName[yytos->major]);
  }
#endif
  yy_destructor(pParser, yytos->major, &yytos->minor);
}

/* 
** Deallocate and destroy a parser.  Destructors are called for
** all stack elements before shutting the parser down.
**
** If the YYPARSEFREENEVERNULL macro exists (for example because it
** is defined in a %include section of the input grammar) then it is
** assumed that the input pointer is never NULL.
*/
void sqlite3ParserFree(
  void *p,                    /* The parser to be deleted */
  void (*freeProc)(void*)     /* Function used to reclaim memory */
){
  yyParser *pParser = (yyParser*)p;
#ifndef YYPARSEFREENEVERNULL
  if( pParser==0 ) return;
#endif
  while( pParser->yytos>pParser->yystack ) yy_pop_parser_stack(pParser);
#if YYSTACKDEPTH<=0
  if( pParser->yystack!=&pParser->yystk0 ) free(pParser->yystack);
#endif
  (*freeProc)((void*)pParser);
}

/*
** Return the peak depth of the stack for a parser.
*/
#ifdef YYTRACKMAXSTACKDEPTH
int sqlite3ParserStackPeak(void *p){
  yyParser *pParser = (yyParser*)p;
  return pParser->yyhwm;
}
#endif

/*
** Find the appropriate action for a parser given the terminal
** look-ahead token iLookAhead.
*/
static unsigned int yy_find_shift_action(
  yyParser *pParser,        /* The parser */
  YYCODETYPE iLookAhead     /* The look-ahead token */
){
  int i;
  int stateno = pParser->yytos->stateno;
 
  if( stateno>=YY_MIN_REDUCE ) return stateno;
  assert( stateno <= YY_SHIFT_COUNT );
  do{
    i = yy_shift_ofst[stateno];
    assert( iLookAhead!=YYNOCODE );
    i += iLookAhead;
    if( i<0 || i>=YY_ACTTAB_COUNT || yy_lookahead[i]!=iLookAhead ){
#ifdef YYFALLBACK
      YYCODETYPE iFallback;            /* Fallback token */
      if( iLookAhead<sizeof(yyFallback)/sizeof(yyFallback[0])
             && (iFallback = yyFallback[iLookAhead])!=0 ){
#ifndef NDEBUG
        if( yyTraceFILE ){
          fprintf(yyTraceFILE, "%sFALLBACK %s => %s\n",
             yyTracePrompt, yyTokenName[iLookAhead], yyTokenName[iFallback]);
        }
#endif
        assert( yyFallback[iFallback]==0 ); /* Fallback loop must terminate */
        iLookAhead = iFallback;
        continue;
      }
#endif
#ifdef YYWILDCARD
      {
        int j = i - iLookAhead + YYWILDCARD;
        if( 
#if YY_SHIFT_MIN+YYWILDCARD<0
          j>=0 &&
#endif
#if YY_SHIFT_MAX+YYWILDCARD>=YY_ACTTAB_COUNT
          j<YY_ACTTAB_COUNT &&
#endif
          yy_lookahead[j]==YYWILDCARD && iLookAhead>0
        ){
#ifndef NDEBUG
          if( yyTraceFILE ){
            fprintf(yyTraceFILE, "%sWILDCARD %s => %s\n",
               yyTracePrompt, yyTokenName[iLookAhead],
               yyTokenName[YYWILDCARD]);
          }
#endif /* NDEBUG */
          return yy_action[j];
        }
      }
#endif /* YYWILDCARD */
      return yy_default[stateno];
    }else{
      return yy_action[i];
    }
  }while(1);
}

/*
** Find the appropriate action for a parser given the non-terminal
** look-ahead token iLookAhead.
*/
static int yy_find_reduce_action(
  int stateno,              /* Current state number */
  YYCODETYPE iLookAhead     /* The look-ahead token */
){
  int i;
#ifdef YYERRORSYMBOL
  if( stateno>YY_REDUCE_COUNT ){
    return yy_default[stateno];
  }
#else
  assert( stateno<=YY_REDUCE_COUNT );
#endif
  i = yy_reduce_ofst[stateno];
  assert( i!=YY_REDUCE_USE_DFLT );
  assert( iLookAhead!=YYNOCODE );
  i += iLookAhead;
#ifdef YYERRORSYMBOL
  if( i<0 || i>=YY_ACTTAB_COUNT || yy_lookahead[i]!=iLookAhead ){
    return yy_default[stateno];
  }
#else
  assert( i>=0 && i<YY_ACTTAB_COUNT );
  assert( yy_lookahead[i]==iLookAhead );
#endif
  return yy_action[i];
}

/*
** The following routine is called if the stack overflows.
*/
static void yyStackOverflow(yyParser *yypParser){
   sqlite3ParserARG_FETCH;
   yypParser->yytos--;
#ifndef NDEBUG
   if( yyTraceFILE ){
     fprintf(yyTraceFILE,"%sStack Overflow!\n",yyTracePrompt);
   }
#endif
   while( yypParser->yytos>yypParser->yystack ) yy_pop_parser_stack(yypParser);
   /* Here code is inserted which will execute if the parser
   ** stack every overflows */
/******** Begin %stack_overflow code ******************************************/
#line 37 "parse.y"

  sqlite3ErrorMsg(pParse, "parser stack overflow");
#line 1621 "parse.c"
/******** End %stack_overflow code ********************************************/
   sqlite3ParserARG_STORE; /* Suppress warning about unused %extra_argument var */
}

/*
** Print tracing information for a SHIFT action
*/
#ifndef NDEBUG
static void yyTraceShift(yyParser *yypParser, int yyNewState){
  if( yyTraceFILE ){
    if( yyNewState<YYNSTATE ){
      fprintf(yyTraceFILE,"%sShift '%s', go to state %d\n",
         yyTracePrompt,yyTokenName[yypParser->yytos->major],
         yyNewState);
    }else{
      fprintf(yyTraceFILE,"%sShift '%s'\n",
         yyTracePrompt,yyTokenName[yypParser->yytos->major]);
    }
  }
}
#else
# define yyTraceShift(X,Y)
#endif

/*
** Perform a shift action.
*/
static void yy_shift(
  yyParser *yypParser,          /* The parser to be shifted */
  int yyNewState,               /* The new state to shift in */
  int yyMajor,                  /* The major token to shift in */
  sqlite3ParserTOKENTYPE yyMinor        /* The minor token to shift in */
){
  yyStackEntry *yytos;
  yypParser->yytos++;
#ifdef YYTRACKMAXSTACKDEPTH
  if( (int)(yypParser->yytos - yypParser->yystack)>yypParser->yyhwm ){
    yypParser->yyhwm++;
    assert( yypParser->yyhwm == (int)(yypParser->yytos - yypParser->yystack) );
  }
#endif
#if YYSTACKDEPTH>0 
  if( yypParser->yytos>=&yypParser->yystack[YYSTACKDEPTH] ){
    yyStackOverflow(yypParser);
    return;
  }
#else
  if( yypParser->yytos>=&yypParser->yystack[yypParser->yystksz] ){
    if( yyGrowStack(yypParser) ){
      yyStackOverflow(yypParser);
      return;
    }
  }
#endif
  if( yyNewState > YY_MAX_SHIFT ){
    yyNewState += YY_MIN_REDUCE - YY_MIN_SHIFTREDUCE;
  }
  yytos = yypParser->yytos;
  yytos->stateno = (YYACTIONTYPE)yyNewState;
  yytos->major = (YYCODETYPE)yyMajor;
  yytos->minor.yy0 = yyMinor;
  yyTraceShift(yypParser, yyNewState);
}

/* The following table contains information about every rule that
** is used during the reduce.
*/
static const struct {
  YYCODETYPE lhs;         /* Symbol on the left-hand side of the rule */
  unsigned char nrhs;     /* Number of right-hand side symbols in the rule */
} yyRuleInfo[] = {
  { 146, 1 },
  { 147, 3 },
  { 148, 0 },
  { 148, 1 },
  { 148, 1 },
  { 148, 1 },
  { 147, 2 },
  { 147, 2 },
  { 147, 2 },
  { 147, 2 },
  { 147, 3 },
  { 147, 5 },
  { 152, 6 },
  { 154, 1 },
  { 156, 0 },
  { 156, 3 },
  { 155, 0 },
  { 153, 5 },
  { 153, 2 },
  { 160, 0 },
  { 160, 2 },
  { 162, 2 },
  { 164, 0 },
  { 164, 4 },
  { 164, 6 },
  { 165, 2 },
  { 169, 2 },
  { 169, 2 },
  { 169, 4 },
  { 169, 3 },
  { 169, 3 },
  { 169, 2 },
  { 169, 3 },
  { 169, 5 },
  { 169, 2 },
  { 169, 4 },
  { 169, 4 },
  { 169, 1 },
  { 169, 2 },
  { 174, 0 },
  { 174, 1 },
  { 176, 0 },
  { 176, 2 },
  { 178, 2 },
  { 178, 3 },
  { 178, 3 },
  { 178, 3 },
  { 179, 2 },
  { 179, 2 },
  { 179, 1 },
  { 179, 1 },
  { 179, 2 },
  { 177, 3 },
  { 177, 2 },
  { 180, 0 },
  { 180, 2 },
  { 180, 2 },
  { 159, 0 },
  { 182, 1 },
  { 183, 2 },
  { 183, 7 },
  { 183, 5 },
  { 183, 5 },
  { 183, 10 },
  { 186, 0 },
  { 172, 0 },
  { 172, 3 },
  { 187, 0 },
  { 187, 2 },
  { 188, 1 },
  { 188, 1 },
  { 147, 4 },
  { 190, 2 },
  { 190, 0 },
  { 147, 1 },
  { 161, 2 },
  { 193, 9 },
  { 203, 4 },
  { 203, 5 },
  { 195, 1 },
  { 195, 1 },
  { 195, 0 },
  { 206, 0 },
  { 196, 3 },
  { 196, 2 },
  { 196, 4 },
  { 207, 2 },
  { 207, 0 },
  { 197, 0 },
  { 197, 2 },
  { 209, 2 },
  { 209, 0 },
  { 208, 7 },
  { 208, 9 },
  { 208, 7 },
  { 208, 7 },
  { 157, 0 },
  { 157, 2 },
  { 191, 2 },
  { 210, 1 },
  { 210, 2 },
  { 210, 3 },
  { 210, 4 },
  { 212, 2 },
  { 212, 0 },
  { 211, 0 },
  { 211, 3 },
  { 211, 2 },
  { 213, 4 },
  { 213, 0 },
  { 201, 0 },
  { 201, 3 },
  { 184, 4 },
  { 184, 2 },
  { 173, 1 },
  { 173, 1 },
  { 173, 0 },
  { 199, 0 },
  { 199, 3 },
  { 200, 0 },
  { 200, 2 },
  { 202, 0 },
  { 202, 2 },
  { 202, 4 },
  { 202, 4 },
  { 147, 6 },
  { 198, 0 },
  { 198, 2 },
  { 147, 8 },
  { 215, 5 },
  { 215, 7 },
  { 215, 3 },
  { 215, 5 },
  { 147, 6 },
  { 147, 7 },
  { 216, 2 },
  { 216, 1 },
  { 217, 0 },
  { 217, 3 },
  { 214, 3 },
  { 214, 1 },
  { 171, 3 },
  { 170, 1 },
  { 171, 1 },
  { 171, 1 },
  { 171, 3 },
  { 171, 5 },
  { 170, 1 },
  { 170, 1 },
  { 170, 1 },
  { 171, 1 },
  { 171, 3 },
  { 171, 5 },
  { 171, 4 },
  { 170, 1 },
  { 171, 5 },
  { 171, 3 },
  { 171, 3 },
  { 171, 3 },
  { 171, 3 },
  { 171, 3 },
  { 171, 3 },
  { 171, 3 },
  { 171, 3 },
  { 218, 1 },
  { 218, 2 },
  { 171, 3 },
  { 171, 5 },
  { 171, 2 },
  { 171, 3 },
  { 171, 3 },
  { 171, 4 },
  { 171, 2 },
  { 171, 2 },
  { 171, 2 },
  { 171, 2 },
  { 219, 1 },
  { 219, 2 },
  { 171, 5 },
  { 220, 1 },
  { 220, 2 },
  { 171, 5 },
  { 171, 3 },
  { 171, 5 },
  { 171, 5 },
  { 171, 4 },
  { 171, 5 },
  { 223, 5 },
  { 223, 4 },
  { 224, 2 },
  { 224, 0 },
  { 222, 1 },
  { 222, 0 },
  { 205, 0 },
  { 204, 3 },
  { 204, 1 },
  { 221, 0 },
  { 221, 3 },
  { 147, 12 },
  { 225, 1 },
  { 225, 0 },
  { 175, 0 },
  { 175, 3 },
  { 185, 5 },
  { 185, 3 },
  { 226, 0 },
  { 226, 2 },
  { 147, 4 },
  { 167, 2 },
  { 168, 2 },
  { 189, 1 },
  { 189, 1 },
  { 189, 1 },
  { 194, 0 },
  { 142, 1 },
  { 143, 2 },
  { 143, 1 },
  { 144, 1 },
  { 144, 3 },
  { 145, 0 },
  { 149, 0 },
  { 149, 1 },
  { 149, 2 },
  { 151, 1 },
  { 151, 0 },
  { 147, 2 },
  { 158, 4 },
  { 158, 2 },
  { 150, 1 },
  { 150, 1 },
  { 150, 1 },
  { 164, 1 },
  { 165, 1 },
  { 166, 1 },
  { 166, 1 },
  { 163, 2 },
  { 163, 0 },
  { 169, 2 },
  { 159, 2 },
  { 181, 3 },
  { 181, 1 },
  { 182, 0 },
  { 186, 1 },
  { 188, 1 },
  { 192, 1 },
  { 193, 1 },
  { 206, 2 },
  { 207, 1 },
  { 171, 1 },
  { 205, 1 },
  { 167, 1 },
};

static void yy_accept(yyParser*);  /* Forward Declaration */

/*
** Perform a reduce action and the shift that must immediately
** follow the reduce.
*/
static void yy_reduce(
  yyParser *yypParser,         /* The parser */
  unsigned int yyruleno        /* Number of the rule by which to reduce */
){
  int yygoto;                     /* The next state */
  int yyact;                      /* The next action */
  yyStackEntry *yymsp;            /* The top of the parser's stack */
  int yysize;                     /* Amount to pop the stack */
  sqlite3ParserARG_FETCH;
  yymsp = yypParser->yytos;
#ifndef NDEBUG
  if( yyTraceFILE && yyruleno<(int)(sizeof(yyRuleName)/sizeof(yyRuleName[0])) ){
    yysize = yyRuleInfo[yyruleno].nrhs;
    fprintf(yyTraceFILE, "%sReduce [%s], go to state %d.\n", yyTracePrompt,
      yyRuleName[yyruleno], yymsp[-yysize].stateno);
  }
#endif /* NDEBUG */

  /* Check that the stack is large enough to grow by a single entry
  ** if the RHS of the rule is empty.  This ensures that there is room
  ** enough on the stack to push the LHS value */
  if( yyRuleInfo[yyruleno].nrhs==0 ){
#ifdef YYTRACKMAXSTACKDEPTH
    if( (int)(yypParser->yytos - yypParser->yystack)>yypParser->yyhwm ){
      yypParser->yyhwm++;
      assert( yypParser->yyhwm == (int)(yypParser->yytos - yypParser->yystack));
    }
#endif
#if YYSTACKDEPTH>0 
    if( yypParser->yytos>=&yypParser->yystack[YYSTACKDEPTH-1] ){
      yyStackOverflow(yypParser);
      return;
    }
#else
    if( yypParser->yytos>=&yypParser->yystack[yypParser->yystksz-1] ){
      if( yyGrowStack(yypParser) ){
        yyStackOverflow(yypParser);
        return;
      }
      yymsp = yypParser->yytos;
    }
#endif
  }

  switch( yyruleno ){
  /* Beginning here are the reduction cases.  A typical example
  ** follows:
  **   case 0:
  **  #line <lineno> <grammarfile>
  **     { ... }           // User supplied code
  **  #line <lineno> <thisfile>
  **     break;
  */
/********** Begin reduce actions **********************************************/
        YYMINORTYPE yylhsminor;
      case 0: /* cmdx ::= cmd */
#line 116 "parse.y"
{ sqlite3FinishCoding(pParse); }
#line 2010 "parse.c"
        break;
      case 1: /* cmd ::= BEGIN transtype trans_opt */
#line 121 "parse.y"
{sqlite3BeginTransaction(pParse, yymsp[-1].minor.yy388);}
#line 2015 "parse.c"
        break;
      case 2: /* transtype ::= */
#line 126 "parse.y"
{yymsp[1].minor.yy388 = TK_DEFERRED;}
#line 2020 "parse.c"
        break;
      case 3: /* transtype ::= DEFERRED */
      case 4: /* transtype ::= IMMEDIATE */ yytestcase(yyruleno==4);
      case 5: /* transtype ::= EXCLUSIVE */ yytestcase(yyruleno==5);
#line 127 "parse.y"
{yymsp[0].minor.yy388 = yymsp[0].major; /*A-overwrites-X*/}
#line 2027 "parse.c"
        break;
      case 6: /* cmd ::= COMMIT trans_opt */
      case 7: /* cmd ::= END trans_opt */ yytestcase(yyruleno==7);
#line 130 "parse.y"
{sqlite3CommitTransaction(pParse);}
#line 2033 "parse.c"
        break;
      case 8: /* cmd ::= ROLLBACK trans_opt */
#line 132 "parse.y"
{sqlite3RollbackTransaction(pParse);}
#line 2038 "parse.c"
        break;
      case 9: /* cmd ::= SAVEPOINT nm */
#line 136 "parse.y"
{
  sqlite3Savepoint(pParse, SAVEPOINT_BEGIN, &yymsp[0].minor.yy0);
}
#line 2045 "parse.c"
        break;
      case 10: /* cmd ::= RELEASE savepoint_opt nm */
#line 139 "parse.y"
{
  sqlite3Savepoint(pParse, SAVEPOINT_RELEASE, &yymsp[0].minor.yy0);
}
#line 2052 "parse.c"
        break;
      case 11: /* cmd ::= ROLLBACK trans_opt TO savepoint_opt nm */
#line 142 "parse.y"
{
  sqlite3Savepoint(pParse, SAVEPOINT_ROLLBACK, &yymsp[0].minor.yy0);
}
#line 2059 "parse.c"
        break;
      case 12: /* create_table ::= createkw temp TABLE ifnotexists nm dbnm */
#line 149 "parse.y"
{
   sqlite3StartTable(pParse,&yymsp[-1].minor.yy0,&yymsp[0].minor.yy0,yymsp[-4].minor.yy388,0,0,yymsp[-2].minor.yy388);
}
#line 2066 "parse.c"
        break;
      case 13: /* createkw ::= CREATE */
#line 152 "parse.y"
{disableLookaside(pParse);}
#line 2071 "parse.c"
        break;
      case 14: /* ifnotexists ::= */
      case 16: /* temp ::= */ yytestcase(yyruleno==16);
      case 19: /* table_options ::= */ yytestcase(yyruleno==19);
      case 39: /* autoinc ::= */ yytestcase(yyruleno==39);
      case 54: /* init_deferred_pred_opt ::= */ yytestcase(yyruleno==54);
      case 64: /* defer_subclause_opt ::= */ yytestcase(yyruleno==64);
      case 73: /* ifexists ::= */ yytestcase(yyruleno==73);
      case 81: /* distinct ::= */ yytestcase(yyruleno==81);
      case 205: /* collate ::= */ yytestcase(yyruleno==205);
#line 155 "parse.y"
{yymsp[1].minor.yy388 = 0;}
#line 2084 "parse.c"
        break;
      case 15: /* ifnotexists ::= IF NOT EXISTS */
#line 156 "parse.y"
{yymsp[-2].minor.yy388 = 1;}
#line 2089 "parse.c"
        break;
      case 17: /* create_table_args ::= LP columnlist conslist_opt RP table_options */
#line 162 "parse.y"
{
  sqlite3EndTable(pParse,&yymsp[-2].minor.yy0,&yymsp[-1].minor.yy0,yymsp[0].minor.yy388,0);
}
#line 2096 "parse.c"
        break;
      case 18: /* create_table_args ::= AS select */
#line 165 "parse.y"
{
  sqlite3EndTable(pParse,0,0,0,yymsp[0].minor.yy219);
  sqlite3SelectDelete(pParse->db, yymsp[0].minor.yy219);
}
#line 2104 "parse.c"
        break;
      case 20: /* table_options ::= WITHOUT nm */
#line 171 "parse.y"
{
  if( yymsp[0].minor.yy0.n==5 && sqlite3_strnicmp(yymsp[0].minor.yy0.z,"rowid",5)==0 ){
    yymsp[-1].minor.yy388 = TF_WithoutRowid | TF_NoVisibleRowid;
  }else{
    yymsp[-1].minor.yy388 = 0;
    sqlite3ErrorMsg(pParse, "unknown table option: %.*s", yymsp[0].minor.yy0.n, yymsp[0].minor.yy0.z);
  }
}
#line 2116 "parse.c"
        break;
      case 21: /* columnname ::= nm typetoken */
#line 181 "parse.y"
{sqlite3AddColumn(pParse,&yymsp[-1].minor.yy0,&yymsp[0].minor.yy0);}
#line 2121 "parse.c"
        break;
      case 22: /* typetoken ::= */
      case 57: /* conslist_opt ::= */ yytestcase(yyruleno==57);
      case 87: /* as ::= */ yytestcase(yyruleno==87);
#line 246 "parse.y"
{yymsp[1].minor.yy0.n = 0; yymsp[1].minor.yy0.z = 0;}
#line 2128 "parse.c"
        break;
      case 23: /* typetoken ::= typename LP signed RP */
#line 248 "parse.y"
{
  yymsp[-3].minor.yy0.n = (int)(&yymsp[0].minor.yy0.z[yymsp[0].minor.yy0.n] - yymsp[-3].minor.yy0.z);
}
#line 2135 "parse.c"
        break;
      case 24: /* typetoken ::= typename LP signed COMMA signed RP */
#line 251 "parse.y"
{
  yymsp[-5].minor.yy0.n = (int)(&yymsp[0].minor.yy0.z[yymsp[0].minor.yy0.n] - yymsp[-5].minor.yy0.z);
}
#line 2142 "parse.c"
        break;
      case 25: /* typename ::= typename ID|STRING */
#line 256 "parse.y"
{yymsp[-1].minor.yy0.n=yymsp[0].minor.yy0.n+(int)(yymsp[0].minor.yy0.z-yymsp[-1].minor.yy0.z);}
#line 2147 "parse.c"
        break;
      case 26: /* ccons ::= CONSTRAINT nm */
      case 59: /* tcons ::= CONSTRAINT nm */ yytestcase(yyruleno==59);
#line 265 "parse.y"
{pParse->constraintName = yymsp[0].minor.yy0;}
#line 2153 "parse.c"
        break;
      case 27: /* ccons ::= DEFAULT term */
      case 29: /* ccons ::= DEFAULT PLUS term */ yytestcase(yyruleno==29);
#line 266 "parse.y"
{sqlite3AddDefaultValue(pParse,&yymsp[0].minor.yy382);}
#line 2159 "parse.c"
        break;
      case 28: /* ccons ::= DEFAULT LP expr RP */
#line 267 "parse.y"
{sqlite3AddDefaultValue(pParse,&yymsp[-1].minor.yy382);}
#line 2164 "parse.c"
        break;
      case 30: /* ccons ::= DEFAULT MINUS term */
#line 269 "parse.y"
{
  ExprSpan v;
  v.pExpr = sqlite3PExpr(pParse, TK_UMINUS, yymsp[0].minor.yy382.pExpr, 0, 0);
  v.zStart = yymsp[-1].minor.yy0.z;
  v.zEnd = yymsp[0].minor.yy382.zEnd;
  sqlite3AddDefaultValue(pParse,&v);
}
#line 2175 "parse.c"
        break;
      case 31: /* ccons ::= DEFAULT ID|INDEXED */
#line 276 "parse.y"
{
  ExprSpan v;
  spanExpr(&v, pParse, TK_STRING, yymsp[0].minor.yy0);
  sqlite3AddDefaultValue(pParse,&v);
}
#line 2184 "parse.c"
        break;
      case 32: /* ccons ::= NOT NULL onconf */
#line 286 "parse.y"
{sqlite3AddNotNull(pParse, yymsp[0].minor.yy388);}
#line 2189 "parse.c"
        break;
      case 33: /* ccons ::= PRIMARY KEY sortorder onconf autoinc */
#line 288 "parse.y"
{sqlite3AddPrimaryKey(pParse,0,yymsp[-1].minor.yy388,yymsp[0].minor.yy388,yymsp[-2].minor.yy388);}
#line 2194 "parse.c"
        break;
      case 34: /* ccons ::= UNIQUE onconf */
#line 289 "parse.y"
{sqlite3CreateIndex(pParse,0,0,0,0,yymsp[0].minor.yy388,0,0,0,0,
                                   SQLITE_IDXTYPE_UNIQUE);}
#line 2200 "parse.c"
        break;
      case 35: /* ccons ::= CHECK LP expr RP */
#line 291 "parse.y"
{sqlite3AddCheckConstraint(pParse,yymsp[-1].minor.yy382.pExpr);}
#line 2205 "parse.c"
        break;
      case 36: /* ccons ::= REFERENCES nm eidlist_opt refargs */
#line 293 "parse.y"
{sqlite3CreateForeignKey(pParse,0,&yymsp[-2].minor.yy0,yymsp[-1].minor.yy250,yymsp[0].minor.yy388);}
#line 2210 "parse.c"
        break;
      case 37: /* ccons ::= defer_subclause */
#line 294 "parse.y"
{sqlite3DeferForeignKey(pParse,yymsp[0].minor.yy388);}
#line 2215 "parse.c"
        break;
      case 38: /* ccons ::= COLLATE ID|STRING */
#line 295 "parse.y"
{sqlite3AddCollateType(pParse, &yymsp[0].minor.yy0);}
#line 2220 "parse.c"
        break;
      case 40: /* autoinc ::= AUTOINCR */
#line 300 "parse.y"
{yymsp[0].minor.yy388 = 1;}
#line 2225 "parse.c"
        break;
      case 41: /* refargs ::= */
#line 308 "parse.y"
{ yymsp[1].minor.yy388 = OE_None*0x0101; /* EV: R-19803-45884 */}
#line 2230 "parse.c"
        break;
      case 42: /* refargs ::= refargs refarg */
#line 309 "parse.y"
{ yymsp[-1].minor.yy388 = (yymsp[-1].minor.yy388 & ~yymsp[0].minor.yy215.mask) | yymsp[0].minor.yy215.value; }
#line 2235 "parse.c"
        break;
      case 43: /* refarg ::= MATCH nm */
#line 311 "parse.y"
{ yymsp[-1].minor.yy215.value = 0;     yymsp[-1].minor.yy215.mask = 0x000000; }
#line 2240 "parse.c"
        break;
      case 44: /* refarg ::= ON INSERT refact */
#line 312 "parse.y"
{ yymsp[-2].minor.yy215.value = 0;     yymsp[-2].minor.yy215.mask = 0x000000; }
#line 2245 "parse.c"
        break;
      case 45: /* refarg ::= ON DELETE refact */
#line 313 "parse.y"
{ yymsp[-2].minor.yy215.value = yymsp[0].minor.yy388;     yymsp[-2].minor.yy215.mask = 0x0000ff; }
#line 2250 "parse.c"
        break;
      case 46: /* refarg ::= ON UPDATE refact */
#line 314 "parse.y"
{ yymsp[-2].minor.yy215.value = yymsp[0].minor.yy388<<8;  yymsp[-2].minor.yy215.mask = 0x00ff00; }
#line 2255 "parse.c"
        break;
      case 47: /* refact ::= SET NULL */
#line 316 "parse.y"
{ yymsp[-1].minor.yy388 = OE_SetNull;  /* EV: R-33326-45252 */}
#line 2260 "parse.c"
        break;
      case 48: /* refact ::= SET DEFAULT */
#line 317 "parse.y"
{ yymsp[-1].minor.yy388 = OE_SetDflt;  /* EV: R-33326-45252 */}
#line 2265 "parse.c"
        break;
      case 49: /* refact ::= CASCADE */
#line 318 "parse.y"
{ yymsp[0].minor.yy388 = OE_Cascade;  /* EV: R-33326-45252 */}
#line 2270 "parse.c"
        break;
      case 50: /* refact ::= RESTRICT */
#line 319 "parse.y"
{ yymsp[0].minor.yy388 = OE_Restrict; /* EV: R-33326-45252 */}
#line 2275 "parse.c"
        break;
      case 51: /* refact ::= NO ACTION */
#line 320 "parse.y"
{ yymsp[-1].minor.yy388 = OE_None;     /* EV: R-33326-45252 */}
#line 2280 "parse.c"
        break;
      case 52: /* defer_subclause ::= NOT DEFERRABLE init_deferred_pred_opt */
#line 322 "parse.y"
{yymsp[-2].minor.yy388 = 0;}
#line 2285 "parse.c"
        break;
      case 53: /* defer_subclause ::= DEFERRABLE init_deferred_pred_opt */
      case 68: /* orconf ::= OR resolvetype */ yytestcase(yyruleno==68);
      case 135: /* insert_cmd ::= INSERT orconf */ yytestcase(yyruleno==135);
#line 323 "parse.y"
{yymsp[-1].minor.yy388 = yymsp[0].minor.yy388;}
#line 2292 "parse.c"
        break;
      case 55: /* init_deferred_pred_opt ::= INITIALLY DEFERRED */
      case 72: /* ifexists ::= IF EXISTS */ yytestcase(yyruleno==72);
      case 177: /* between_op ::= NOT BETWEEN */ yytestcase(yyruleno==177);
      case 180: /* in_op ::= NOT IN */ yytestcase(yyruleno==180);
      case 206: /* collate ::= COLLATE ID|STRING */ yytestcase(yyruleno==206);
#line 326 "parse.y"
{yymsp[-1].minor.yy388 = 1;}
#line 2301 "parse.c"
        break;
      case 56: /* init_deferred_pred_opt ::= INITIALLY IMMEDIATE */
#line 327 "parse.y"
{yymsp[-1].minor.yy388 = 0;}
#line 2306 "parse.c"
        break;
      case 58: /* tconscomma ::= COMMA */
#line 333 "parse.y"
{pParse->constraintName.n = 0;}
#line 2311 "parse.c"
        break;
      case 60: /* tcons ::= PRIMARY KEY LP sortlist autoinc RP onconf */
#line 337 "parse.y"
{sqlite3AddPrimaryKey(pParse,yymsp[-3].minor.yy250,yymsp[0].minor.yy388,yymsp[-2].minor.yy388,0);}
#line 2316 "parse.c"
        break;
      case 61: /* tcons ::= UNIQUE LP sortlist RP onconf */
#line 339 "parse.y"
{sqlite3CreateIndex(pParse,0,0,0,yymsp[-2].minor.yy250,yymsp[0].minor.yy388,0,0,0,0,
                                       SQLITE_IDXTYPE_UNIQUE);}
#line 2322 "parse.c"
        break;
      case 62: /* tcons ::= CHECK LP expr RP onconf */
#line 342 "parse.y"
{sqlite3AddCheckConstraint(pParse,yymsp[-2].minor.yy382.pExpr);}
#line 2327 "parse.c"
        break;
      case 63: /* tcons ::= FOREIGN KEY LP eidlist RP REFERENCES nm eidlist_opt refargs defer_subclause_opt */
#line 344 "parse.y"
{
    sqlite3CreateForeignKey(pParse, yymsp[-6].minor.yy250, &yymsp[-3].minor.yy0, yymsp[-2].minor.yy250, yymsp[-1].minor.yy388);
    sqlite3DeferForeignKey(pParse, yymsp[0].minor.yy388);
}
#line 2335 "parse.c"
        break;
      case 65: /* onconf ::= */
      case 67: /* orconf ::= */ yytestcase(yyruleno==67);
#line 358 "parse.y"
{yymsp[1].minor.yy388 = OE_Default;}
#line 2341 "parse.c"
        break;
      case 66: /* onconf ::= ON CONFLICT resolvetype */
#line 359 "parse.y"
{yymsp[-2].minor.yy388 = yymsp[0].minor.yy388;}
#line 2346 "parse.c"
        break;
      case 69: /* resolvetype ::= IGNORE */
#line 363 "parse.y"
{yymsp[0].minor.yy388 = OE_Ignore;}
#line 2351 "parse.c"
        break;
      case 70: /* resolvetype ::= REPLACE */
      case 136: /* insert_cmd ::= REPLACE */ yytestcase(yyruleno==136);
#line 364 "parse.y"
{yymsp[0].minor.yy388 = OE_Replace;}
#line 2357 "parse.c"
        break;
      case 71: /* cmd ::= DROP TABLE ifexists fullname */
#line 368 "parse.y"
{
  sqlite3DropTable(pParse, yymsp[0].minor.yy259, 0, yymsp[-1].minor.yy388);
}
#line 2364 "parse.c"
        break;
      case 74: /* cmd ::= select */
#line 389 "parse.y"
{
  SelectDest dest = {SRT_Output, 0, 0, 0, 0, 0};
  sqlite3Select(pParse, yymsp[0].minor.yy219, &dest);
  sqlite3SelectDelete(pParse->db, yymsp[0].minor.yy219);
}
#line 2373 "parse.c"
        break;
      case 75: /* select ::= with selectnowith */
#line 426 "parse.y"
{
  Select *p = yymsp[0].minor.yy219;
  if( p ){
    p->pWith = yymsp[-1].minor.yy163;
    parserDoubleLinkSelect(pParse, p);
  }else{
    sqlite3WithDelete(pParse->db, yymsp[-1].minor.yy163);
  }
  yymsp[-1].minor.yy219 = p; /*A-overwrites-W*/
}
#line 2387 "parse.c"
        break;
      case 76: /* oneselect ::= SELECT distinct selcollist from where_opt groupby_opt having_opt orderby_opt limit_opt */
#line 467 "parse.y"
{
#if SELECTTRACE_ENABLED
  Token s = yymsp[-8].minor.yy0; /*A-overwrites-S*/
#endif
  yymsp[-8].minor.yy219 = sqlite3SelectNew(pParse,yymsp[-6].minor.yy250,yymsp[-5].minor.yy259,yymsp[-4].minor.yy362,yymsp[-3].minor.yy250,yymsp[-2].minor.yy362,yymsp[-1].minor.yy250,yymsp[-7].minor.yy388,yymsp[0].minor.yy196.pLimit,yymsp[0].minor.yy196.pOffset);
#if SELECTTRACE_ENABLED
  /* Populate the Select.zSelName[] string that is used to help with
  ** query planner debugging, to differentiate between multiple Select
  ** objects in a complex query.
  **
  ** If the SELECT keyword is immediately followed by a C-style comment
  ** then extract the first few alphanumeric characters from within that
  ** comment to be the zSelName value.  Otherwise, the label is #N where
  ** is an integer that is incremented with each SELECT statement seen.
  */
  if( yymsp[-8].minor.yy219!=0 ){
    const char *z = s.z+6;
    int i;
    sqlite3_snprintf(sizeof(yymsp[-8].minor.yy219->zSelName), yymsp[-8].minor.yy219->zSelName, "#%d",
                     ++pParse->nSelect);
    while( z[0]==' ' ) z++;
    if( z[0]=='/' && z[1]=='*' ){
      z += 2;
      while( z[0]==' ' ) z++;
      for(i=0; sqlite3Isalnum(z[i]); i++){}
      sqlite3_snprintf(sizeof(yymsp[-8].minor.yy219->zSelName), yymsp[-8].minor.yy219->zSelName, "%.*s", i, z);
    }
  }
#endif /* SELECTRACE_ENABLED */
}
#line 2421 "parse.c"
        break;
      case 77: /* values ::= VALUES LP nexprlist RP */
#line 501 "parse.y"
{
  yymsp[-3].minor.yy219 = sqlite3SelectNew(pParse,yymsp[-1].minor.yy250,0,0,0,0,0,SF_Values,0,0);
}
#line 2428 "parse.c"
        break;
      case 78: /* values ::= values COMMA LP exprlist RP */
#line 504 "parse.y"
{
  Select *pRight, *pLeft = yymsp[-4].minor.yy219;
  pRight = sqlite3SelectNew(pParse,yymsp[-1].minor.yy250,0,0,0,0,0,SF_Values|SF_MultiValue,0,0);
  if( ALWAYS(pLeft) ) pLeft->selFlags &= ~SF_MultiValue;
  if( pRight ){
    pRight->op = TK_ALL;
    pRight->pPrior = pLeft;
    yymsp[-4].minor.yy219 = pRight;
  }else{
    yymsp[-4].minor.yy219 = pLeft;
  }
}
#line 2444 "parse.c"
        break;
      case 79: /* distinct ::= DISTINCT */
#line 521 "parse.y"
{yymsp[0].minor.yy388 = SF_Distinct;}
#line 2449 "parse.c"
        break;
      case 80: /* distinct ::= ALL */
#line 522 "parse.y"
{yymsp[0].minor.yy388 = SF_All;}
#line 2454 "parse.c"
        break;
      case 82: /* sclp ::= */
      case 110: /* orderby_opt ::= */ yytestcase(yyruleno==110);
      case 117: /* groupby_opt ::= */ yytestcase(yyruleno==117);
      case 193: /* exprlist ::= */ yytestcase(yyruleno==193);
      case 196: /* paren_exprlist ::= */ yytestcase(yyruleno==196);
      case 201: /* eidlist_opt ::= */ yytestcase(yyruleno==201);
#line 535 "parse.y"
{yymsp[1].minor.yy250 = 0;}
#line 2464 "parse.c"
        break;
      case 83: /* selcollist ::= sclp expr as */
#line 536 "parse.y"
{
   yymsp[-2].minor.yy250 = sqlite3ExprListAppend(pParse, yymsp[-2].minor.yy250, yymsp[-1].minor.yy382.pExpr);
   if( yymsp[0].minor.yy0.n>0 ) sqlite3ExprListSetName(pParse, yymsp[-2].minor.yy250, &yymsp[0].minor.yy0, 1);
   sqlite3ExprListSetSpan(pParse,yymsp[-2].minor.yy250,&yymsp[-1].minor.yy382);
}
#line 2473 "parse.c"
        break;
      case 84: /* selcollist ::= sclp STAR */
#line 541 "parse.y"
{
  Expr *p = sqlite3Expr(pParse->db, TK_ASTERISK, 0);
  yymsp[-1].minor.yy250 = sqlite3ExprListAppend(pParse, yymsp[-1].minor.yy250, p);
}
#line 2481 "parse.c"
        break;
      case 85: /* selcollist ::= sclp nm DOT STAR */
#line 545 "parse.y"
{
  Expr *pRight = sqlite3PExpr(pParse, TK_ASTERISK, 0, 0, 0);
  Expr *pLeft = sqlite3PExpr(pParse, TK_ID, 0, 0, &yymsp[-2].minor.yy0);
  Expr *pDot = sqlite3PExpr(pParse, TK_DOT, pLeft, pRight, 0);
  yymsp[-3].minor.yy250 = sqlite3ExprListAppend(pParse,yymsp[-3].minor.yy250, pDot);
}
#line 2491 "parse.c"
        break;
      case 86: /* as ::= AS nm */
      case 97: /* dbnm ::= DOT nm */ yytestcase(yyruleno==97);
      case 208: /* plus_num ::= PLUS INTEGER|FLOAT */ yytestcase(yyruleno==208);
      case 209: /* minus_num ::= MINUS INTEGER|FLOAT */ yytestcase(yyruleno==209);
#line 556 "parse.y"
{yymsp[-1].minor.yy0 = yymsp[0].minor.yy0;}
#line 2499 "parse.c"
        break;
      case 88: /* from ::= */
#line 570 "parse.y"
{yymsp[1].minor.yy259 = sqlite3DbMallocZero(pParse->db, sizeof(*yymsp[1].minor.yy259));}
#line 2504 "parse.c"
        break;
      case 89: /* from ::= FROM seltablist */
#line 571 "parse.y"
{
  yymsp[-1].minor.yy259 = yymsp[0].minor.yy259;
  sqlite3SrcListShiftJoinType(yymsp[-1].minor.yy259);
}
#line 2512 "parse.c"
        break;
      case 90: /* stl_prefix ::= seltablist joinop */
#line 579 "parse.y"
{
   if( ALWAYS(yymsp[-1].minor.yy259 && yymsp[-1].minor.yy259->nSrc>0) ) yymsp[-1].minor.yy259->a[yymsp[-1].minor.yy259->nSrc-1].fg.jointype = (u8)yymsp[0].minor.yy388;
}
#line 2519 "parse.c"
        break;
      case 91: /* stl_prefix ::= */
#line 582 "parse.y"
{yymsp[1].minor.yy259 = 0;}
#line 2524 "parse.c"
        break;
      case 92: /* seltablist ::= stl_prefix nm dbnm as indexed_opt on_opt using_opt */
#line 584 "parse.y"
{
  yymsp[-6].minor.yy259 = sqlite3SrcListAppendFromTerm(pParse,yymsp[-6].minor.yy259,&yymsp[-5].minor.yy0,&yymsp[-4].minor.yy0,&yymsp[-3].minor.yy0,0,yymsp[-1].minor.yy362,yymsp[0].minor.yy432);
  sqlite3SrcListIndexedBy(pParse, yymsp[-6].minor.yy259, &yymsp[-2].minor.yy0);
}
#line 2532 "parse.c"
        break;
      case 93: /* seltablist ::= stl_prefix nm dbnm LP exprlist RP as on_opt using_opt */
#line 589 "parse.y"
{
  yymsp[-8].minor.yy259 = sqlite3SrcListAppendFromTerm(pParse,yymsp[-8].minor.yy259,&yymsp[-7].minor.yy0,&yymsp[-6].minor.yy0,&yymsp[-2].minor.yy0,0,yymsp[-1].minor.yy362,yymsp[0].minor.yy432);
  sqlite3SrcListFuncArgs(pParse, yymsp[-8].minor.yy259, yymsp[-4].minor.yy250);
}
#line 2540 "parse.c"
        break;
      case 94: /* seltablist ::= stl_prefix LP select RP as on_opt using_opt */
#line 595 "parse.y"
{
    yymsp[-6].minor.yy259 = sqlite3SrcListAppendFromTerm(pParse,yymsp[-6].minor.yy259,0,0,&yymsp[-2].minor.yy0,yymsp[-4].minor.yy219,yymsp[-1].minor.yy362,yymsp[0].minor.yy432);
  }
#line 2547 "parse.c"
        break;
      case 95: /* seltablist ::= stl_prefix LP seltablist RP as on_opt using_opt */
#line 599 "parse.y"
{
    if( yymsp[-6].minor.yy259==0 && yymsp[-2].minor.yy0.n==0 && yymsp[-1].minor.yy362==0 && yymsp[0].minor.yy432==0 ){
      yymsp[-6].minor.yy259 = yymsp[-4].minor.yy259;
    }else if( yymsp[-4].minor.yy259->nSrc==1 ){
      yymsp[-6].minor.yy259 = sqlite3SrcListAppendFromTerm(pParse,yymsp[-6].minor.yy259,0,0,&yymsp[-2].minor.yy0,0,yymsp[-1].minor.yy362,yymsp[0].minor.yy432);
      if( yymsp[-6].minor.yy259 ){
        struct SrcList_item *pNew = &yymsp[-6].minor.yy259->a[yymsp[-6].minor.yy259->nSrc-1];
        struct SrcList_item *pOld = yymsp[-4].minor.yy259->a;
        pNew->zName = pOld->zName;
        pNew->zDatabase = pOld->zDatabase;
        pNew->pSelect = pOld->pSelect;
        pOld->zName = pOld->zDatabase = 0;
        pOld->pSelect = 0;
      }
      sqlite3SrcListDelete(pParse->db, yymsp[-4].minor.yy259);
    }else{
      Select *pSubquery;
      sqlite3SrcListShiftJoinType(yymsp[-4].minor.yy259);
      pSubquery = sqlite3SelectNew(pParse,0,yymsp[-4].minor.yy259,0,0,0,0,SF_NestedFrom,0,0);
      yymsp[-6].minor.yy259 = sqlite3SrcListAppendFromTerm(pParse,yymsp[-6].minor.yy259,0,0,&yymsp[-2].minor.yy0,pSubquery,yymsp[-1].minor.yy362,yymsp[0].minor.yy432);
    }
  }
#line 2573 "parse.c"
        break;
      case 96: /* dbnm ::= */
      case 105: /* indexed_opt ::= */ yytestcase(yyruleno==105);
#line 624 "parse.y"
{yymsp[1].minor.yy0.z=0; yymsp[1].minor.yy0.n=0;}
#line 2579 "parse.c"
        break;
      case 98: /* fullname ::= nm dbnm */
#line 630 "parse.y"
{yymsp[-1].minor.yy259 = sqlite3SrcListAppend(pParse->db,0,&yymsp[-1].minor.yy0,&yymsp[0].minor.yy0); /*A-overwrites-X*/}
#line 2584 "parse.c"
        break;
      case 99: /* joinop ::= COMMA|JOIN */
#line 633 "parse.y"
{ yymsp[0].minor.yy388 = JT_INNER; }
#line 2589 "parse.c"
        break;
      case 100: /* joinop ::= JOIN_KW JOIN */
#line 635 "parse.y"
{yymsp[-1].minor.yy388 = sqlite3JoinType(pParse,&yymsp[-1].minor.yy0,0,0);  /*X-overwrites-A*/}
#line 2594 "parse.c"
        break;
      case 101: /* joinop ::= JOIN_KW nm JOIN */
#line 637 "parse.y"
{yymsp[-2].minor.yy388 = sqlite3JoinType(pParse,&yymsp[-2].minor.yy0,&yymsp[-1].minor.yy0,0); /*X-overwrites-A*/}
#line 2599 "parse.c"
        break;
      case 102: /* joinop ::= JOIN_KW nm nm JOIN */
#line 639 "parse.y"
{yymsp[-3].minor.yy388 = sqlite3JoinType(pParse,&yymsp[-3].minor.yy0,&yymsp[-2].minor.yy0,&yymsp[-1].minor.yy0);/*X-overwrites-A*/}
#line 2604 "parse.c"
        break;
      case 103: /* on_opt ::= ON expr */
      case 120: /* having_opt ::= HAVING expr */ yytestcase(yyruleno==120);
      case 127: /* where_opt ::= WHERE expr */ yytestcase(yyruleno==127);
      case 189: /* case_else ::= ELSE expr */ yytestcase(yyruleno==189);
#line 643 "parse.y"
{yymsp[-1].minor.yy362 = yymsp[0].minor.yy382.pExpr;}
#line 2612 "parse.c"
        break;
      case 104: /* on_opt ::= */
      case 119: /* having_opt ::= */ yytestcase(yyruleno==119);
      case 126: /* where_opt ::= */ yytestcase(yyruleno==126);
      case 190: /* case_else ::= */ yytestcase(yyruleno==190);
      case 192: /* case_operand ::= */ yytestcase(yyruleno==192);
#line 644 "parse.y"
{yymsp[1].minor.yy362 = 0;}
#line 2621 "parse.c"
        break;
      case 106: /* indexed_opt ::= INDEXED BY nm */
#line 658 "parse.y"
{yymsp[-2].minor.yy0 = yymsp[0].minor.yy0;}
#line 2626 "parse.c"
        break;
      case 107: /* indexed_opt ::= NOT INDEXED */
#line 659 "parse.y"
{yymsp[-1].minor.yy0.z=0; yymsp[-1].minor.yy0.n=1;}
#line 2631 "parse.c"
        break;
      case 108: /* using_opt ::= USING LP idlist RP */
#line 663 "parse.y"
{yymsp[-3].minor.yy432 = yymsp[-1].minor.yy432;}
#line 2636 "parse.c"
        break;
      case 109: /* using_opt ::= */
      case 137: /* idlist_opt ::= */ yytestcase(yyruleno==137);
#line 664 "parse.y"
{yymsp[1].minor.yy432 = 0;}
#line 2642 "parse.c"
        break;
      case 111: /* orderby_opt ::= ORDER BY sortlist */
      case 118: /* groupby_opt ::= GROUP BY nexprlist */ yytestcase(yyruleno==118);
#line 678 "parse.y"
{yymsp[-2].minor.yy250 = yymsp[0].minor.yy250;}
#line 2648 "parse.c"
        break;
      case 112: /* sortlist ::= sortlist COMMA expr sortorder */
#line 679 "parse.y"
{
  yymsp[-3].minor.yy250 = sqlite3ExprListAppend(pParse,yymsp[-3].minor.yy250,yymsp[-1].minor.yy382.pExpr);
  sqlite3ExprListSetSortOrder(yymsp[-3].minor.yy250,yymsp[0].minor.yy388);
}
#line 2656 "parse.c"
        break;
      case 113: /* sortlist ::= expr sortorder */
#line 683 "parse.y"
{
  yymsp[-1].minor.yy250 = sqlite3ExprListAppend(pParse,0,yymsp[-1].minor.yy382.pExpr); /*A-overwrites-Y*/
  sqlite3ExprListSetSortOrder(yymsp[-1].minor.yy250,yymsp[0].minor.yy388);
}
#line 2664 "parse.c"
        break;
      case 114: /* sortorder ::= ASC */
#line 690 "parse.y"
{yymsp[0].minor.yy388 = SQLITE_SO_ASC;}
#line 2669 "parse.c"
        break;
      case 115: /* sortorder ::= DESC */
#line 691 "parse.y"
{yymsp[0].minor.yy388 = SQLITE_SO_DESC;}
#line 2674 "parse.c"
        break;
      case 116: /* sortorder ::= */
#line 692 "parse.y"
{yymsp[1].minor.yy388 = SQLITE_SO_UNDEFINED;}
#line 2679 "parse.c"
        break;
      case 121: /* limit_opt ::= */
#line 717 "parse.y"
{yymsp[1].minor.yy196.pLimit = 0; yymsp[1].minor.yy196.pOffset = 0;}
#line 2684 "parse.c"
        break;
      case 122: /* limit_opt ::= LIMIT expr */
#line 718 "parse.y"
{yymsp[-1].minor.yy196.pLimit = yymsp[0].minor.yy382.pExpr; yymsp[-1].minor.yy196.pOffset = 0;}
#line 2689 "parse.c"
        break;
      case 123: /* limit_opt ::= LIMIT expr OFFSET expr */
#line 720 "parse.y"
{yymsp[-3].minor.yy196.pLimit = yymsp[-2].minor.yy382.pExpr; yymsp[-3].minor.yy196.pOffset = yymsp[0].minor.yy382.pExpr;}
#line 2694 "parse.c"
        break;
      case 124: /* limit_opt ::= LIMIT expr COMMA expr */
#line 722 "parse.y"
{yymsp[-3].minor.yy196.pOffset = yymsp[-2].minor.yy382.pExpr; yymsp[-3].minor.yy196.pLimit = yymsp[0].minor.yy382.pExpr;}
#line 2699 "parse.c"
        break;
      case 125: /* cmd ::= with DELETE FROM fullname indexed_opt where_opt */
#line 736 "parse.y"
{
  sqlite3WithPush(pParse, yymsp[-5].minor.yy163, 1);
  sqlite3SrcListIndexedBy(pParse, yymsp[-2].minor.yy259, &yymsp[-1].minor.yy0);
  sqlite3DeleteFrom(pParse,yymsp[-2].minor.yy259,yymsp[0].minor.yy362);
}
#line 2708 "parse.c"
        break;
      case 128: /* cmd ::= with UPDATE orconf fullname indexed_opt SET setlist where_opt */
#line 763 "parse.y"
{
  sqlite3WithPush(pParse, yymsp[-7].minor.yy163, 1);
  sqlite3SrcListIndexedBy(pParse, yymsp[-4].minor.yy259, &yymsp[-3].minor.yy0);
  sqlite3ExprListCheckLength(pParse,yymsp[-1].minor.yy250,"set list"); 
  sqlite3Update(pParse,yymsp[-4].minor.yy259,yymsp[-1].minor.yy250,yymsp[0].minor.yy362,yymsp[-5].minor.yy388);
}
#line 2718 "parse.c"
        break;
      case 129: /* setlist ::= setlist COMMA nm EQ expr */
#line 774 "parse.y"
{
  yymsp[-4].minor.yy250 = sqlite3ExprListAppend(pParse, yymsp[-4].minor.yy250, yymsp[0].minor.yy382.pExpr);
  sqlite3ExprListSetName(pParse, yymsp[-4].minor.yy250, &yymsp[-2].minor.yy0, 1);
}
#line 2726 "parse.c"
        break;
      case 130: /* setlist ::= setlist COMMA LP idlist RP EQ expr */
#line 778 "parse.y"
{
  yymsp[-6].minor.yy250 = sqlite3ExprListAppendVector(pParse, yymsp[-6].minor.yy250, yymsp[-3].minor.yy432, yymsp[0].minor.yy382.pExpr);
}
#line 2733 "parse.c"
        break;
      case 131: /* setlist ::= nm EQ expr */
#line 781 "parse.y"
{
  yylhsminor.yy250 = sqlite3ExprListAppend(pParse, 0, yymsp[0].minor.yy382.pExpr);
  sqlite3ExprListSetName(pParse, yylhsminor.yy250, &yymsp[-2].minor.yy0, 1);
}
#line 2741 "parse.c"
  yymsp[-2].minor.yy250 = yylhsminor.yy250;
        break;
      case 132: /* setlist ::= LP idlist RP EQ expr */
#line 785 "parse.y"
{
  yymsp[-4].minor.yy250 = sqlite3ExprListAppendVector(pParse, 0, yymsp[-3].minor.yy432, yymsp[0].minor.yy382.pExpr);
}
#line 2749 "parse.c"
        break;
      case 133: /* cmd ::= with insert_cmd INTO fullname idlist_opt select */
#line 791 "parse.y"
{
  sqlite3WithPush(pParse, yymsp[-5].minor.yy163, 1);
  sqlite3Insert(pParse, yymsp[-2].minor.yy259, yymsp[0].minor.yy219, yymsp[-1].minor.yy432, yymsp[-4].minor.yy388);
}
#line 2757 "parse.c"
        break;
      case 134: /* cmd ::= with insert_cmd INTO fullname idlist_opt DEFAULT VALUES */
#line 796 "parse.y"
{
  sqlite3WithPush(pParse, yymsp[-6].minor.yy163, 1);
  sqlite3Insert(pParse, yymsp[-3].minor.yy259, 0, yymsp[-2].minor.yy432, yymsp[-5].minor.yy388);
}
#line 2765 "parse.c"
        break;
      case 138: /* idlist_opt ::= LP idlist RP */
#line 811 "parse.y"
{yymsp[-2].minor.yy432 = yymsp[-1].minor.yy432;}
#line 2770 "parse.c"
        break;
      case 139: /* idlist ::= idlist COMMA nm */
#line 813 "parse.y"
{yymsp[-2].minor.yy432 = sqlite3IdListAppend(pParse->db,yymsp[-2].minor.yy432,&yymsp[0].minor.yy0);}
#line 2775 "parse.c"
        break;
      case 140: /* idlist ::= nm */
#line 815 "parse.y"
{yymsp[0].minor.yy432 = sqlite3IdListAppend(pParse->db,0,&yymsp[0].minor.yy0); /*A-overwrites-Y*/}
#line 2780 "parse.c"
        break;
      case 141: /* expr ::= LP expr RP */
#line 865 "parse.y"
{spanSet(&yymsp[-2].minor.yy382,&yymsp[-2].minor.yy0,&yymsp[0].minor.yy0); /*A-overwrites-B*/  yymsp[-2].minor.yy382.pExpr = yymsp[-1].minor.yy382.pExpr;}
#line 2785 "parse.c"
        break;
      case 142: /* term ::= NULL */
      case 147: /* term ::= FLOAT|BLOB */ yytestcase(yyruleno==147);
      case 148: /* term ::= STRING */ yytestcase(yyruleno==148);
#line 866 "parse.y"
{spanExpr(&yymsp[0].minor.yy382,pParse,yymsp[0].major,yymsp[0].minor.yy0);/*A-overwrites-X*/}
#line 2792 "parse.c"
        break;
      case 143: /* expr ::= ID|INDEXED */
      case 144: /* expr ::= JOIN_KW */ yytestcase(yyruleno==144);
#line 867 "parse.y"
{spanExpr(&yymsp[0].minor.yy382,pParse,TK_ID,yymsp[0].minor.yy0); /*A-overwrites-X*/}
#line 2798 "parse.c"
        break;
      case 145: /* expr ::= nm DOT nm */
#line 869 "parse.y"
{
  Expr *temp1 = sqlite3ExprAlloc(pParse->db, TK_ID, &yymsp[-2].minor.yy0, 1);
  Expr *temp2 = sqlite3ExprAlloc(pParse->db, TK_ID, &yymsp[0].minor.yy0, 1);
  spanSet(&yymsp[-2].minor.yy382,&yymsp[-2].minor.yy0,&yymsp[0].minor.yy0); /*A-overwrites-X*/
  yymsp[-2].minor.yy382.pExpr = sqlite3PExpr(pParse, TK_DOT, temp1, temp2, 0);
}
#line 2808 "parse.c"
        break;
      case 146: /* expr ::= nm DOT nm DOT nm */
#line 875 "parse.y"
{
  Expr *temp1 = sqlite3ExprAlloc(pParse->db, TK_ID, &yymsp[-4].minor.yy0, 1);
  Expr *temp2 = sqlite3ExprAlloc(pParse->db, TK_ID, &yymsp[-2].minor.yy0, 1);
  Expr *temp3 = sqlite3ExprAlloc(pParse->db, TK_ID, &yymsp[0].minor.yy0, 1);
  Expr *temp4 = sqlite3PExpr(pParse, TK_DOT, temp2, temp3, 0);
  spanSet(&yymsp[-4].minor.yy382,&yymsp[-4].minor.yy0,&yymsp[0].minor.yy0); /*A-overwrites-X*/
  yymsp[-4].minor.yy382.pExpr = sqlite3PExpr(pParse, TK_DOT, temp1, temp4, 0);
}
#line 2820 "parse.c"
        break;
      case 149: /* term ::= INTEGER */
#line 885 "parse.y"
{
  yylhsminor.yy382.pExpr = sqlite3ExprAlloc(pParse->db, TK_INTEGER, &yymsp[0].minor.yy0, 1);
  yylhsminor.yy382.zStart = yymsp[0].minor.yy0.z;
  yylhsminor.yy382.zEnd = yymsp[0].minor.yy0.z + yymsp[0].minor.yy0.n;
  if( yylhsminor.yy382.pExpr ) yylhsminor.yy382.pExpr->flags |= EP_Leaf;
}
#line 2830 "parse.c"
  yymsp[0].minor.yy382 = yylhsminor.yy382;
        break;
      case 150: /* expr ::= VARIABLE */
#line 891 "parse.y"
{
  if( !(yymsp[0].minor.yy0.z[0]=='#' && sqlite3Isdigit(yymsp[0].minor.yy0.z[1])) ){
    u32 n = yymsp[0].minor.yy0.n;
    spanExpr(&yymsp[0].minor.yy382, pParse, TK_VARIABLE, yymsp[0].minor.yy0);
    sqlite3ExprAssignVarNumber(pParse, yymsp[0].minor.yy382.pExpr, n);
  }else{
    /* When doing a nested parse, one can include terms in an expression
    ** that look like this:   #1 #2 ...  These terms refer to registers
    ** in the virtual machine.  #N is the N-th register. */
    Token t = yymsp[0].minor.yy0; /*A-overwrites-X*/
    assert( t.n>=2 );
    spanSet(&yymsp[0].minor.yy382, &t, &t);
    if( pParse->nested==0 ){
      sqlite3ErrorMsg(pParse, "near \"%T\": syntax error", &t);
      yymsp[0].minor.yy382.pExpr = 0;
    }else{
      yymsp[0].minor.yy382.pExpr = sqlite3PExpr(pParse, TK_REGISTER, 0, 0, 0);
      if( yymsp[0].minor.yy382.pExpr ) sqlite3GetInt32(&t.z[1], &yymsp[0].minor.yy382.pExpr->iTable);
    }
  }
}
#line 2856 "parse.c"
        break;
      case 151: /* expr ::= expr COLLATE ID|STRING */
#line 912 "parse.y"
{
  yymsp[-2].minor.yy382.pExpr = sqlite3ExprAddCollateToken(pParse, yymsp[-2].minor.yy382.pExpr, &yymsp[0].minor.yy0, 1);
  yymsp[-2].minor.yy382.zEnd = &yymsp[0].minor.yy0.z[yymsp[0].minor.yy0.n];
}
#line 2864 "parse.c"
        break;
      case 152: /* expr ::= ID|INDEXED LP distinct exprlist RP */
#line 922 "parse.y"
{
  if( yymsp[-1].minor.yy250 && yymsp[-1].minor.yy250->nExpr>pParse->db->aLimit[SQLITE_LIMIT_FUNCTION_ARG] ){
    sqlite3ErrorMsg(pParse, "too many arguments on function %T", &yymsp[-4].minor.yy0);
  }
  yylhsminor.yy382.pExpr = sqlite3ExprFunction(pParse, yymsp[-1].minor.yy250, &yymsp[-4].minor.yy0);
  spanSet(&yylhsminor.yy382,&yymsp[-4].minor.yy0,&yymsp[0].minor.yy0);
  if( yymsp[-2].minor.yy388==SF_Distinct && yylhsminor.yy382.pExpr ){
    yylhsminor.yy382.pExpr->flags |= EP_Distinct;
  }
}
#line 2878 "parse.c"
  yymsp[-4].minor.yy382 = yylhsminor.yy382;
        break;
      case 153: /* expr ::= ID|INDEXED LP STAR RP */
#line 932 "parse.y"
{
  yylhsminor.yy382.pExpr = sqlite3ExprFunction(pParse, 0, &yymsp[-3].minor.yy0);
  spanSet(&yylhsminor.yy382,&yymsp[-3].minor.yy0,&yymsp[0].minor.yy0);
}
#line 2887 "parse.c"
  yymsp[-3].minor.yy382 = yylhsminor.yy382;
        break;
      case 154: /* term ::= CTIME_KW */
#line 936 "parse.y"
{
  yylhsminor.yy382.pExpr = sqlite3ExprFunction(pParse, 0, &yymsp[0].minor.yy0);
  spanSet(&yylhsminor.yy382, &yymsp[0].minor.yy0, &yymsp[0].minor.yy0);
}
#line 2896 "parse.c"
  yymsp[0].minor.yy382 = yylhsminor.yy382;
        break;
      case 155: /* expr ::= LP nexprlist COMMA expr RP */
#line 965 "parse.y"
{
  ExprList *pList = sqlite3ExprListAppend(pParse, yymsp[-3].minor.yy250, yymsp[-1].minor.yy382.pExpr);
  yylhsminor.yy382.pExpr = sqlite3PExpr(pParse, TK_VECTOR, 0, 0, 0);
  if( yylhsminor.yy382.pExpr ){
    yylhsminor.yy382.pExpr->x.pList = pList;
    spanSet(&yylhsminor.yy382, &yymsp[-4].minor.yy0, &yymsp[0].minor.yy0);
  }else{
    sqlite3ExprListDelete(pParse->db, pList);
  }
}
#line 2911 "parse.c"
  yymsp[-4].minor.yy382 = yylhsminor.yy382;
        break;
      case 156: /* expr ::= expr AND expr */
      case 157: /* expr ::= expr OR expr */ yytestcase(yyruleno==157);
      case 158: /* expr ::= expr LT|GT|GE|LE expr */ yytestcase(yyruleno==158);
      case 159: /* expr ::= expr EQ|NE expr */ yytestcase(yyruleno==159);
      case 160: /* expr ::= expr BITAND|BITOR|LSHIFT|RSHIFT expr */ yytestcase(yyruleno==160);
      case 161: /* expr ::= expr PLUS|MINUS expr */ yytestcase(yyruleno==161);
      case 162: /* expr ::= expr STAR|SLASH|REM expr */ yytestcase(yyruleno==162);
      case 163: /* expr ::= expr CONCAT expr */ yytestcase(yyruleno==163);
#line 976 "parse.y"
{spanBinaryExpr(pParse,yymsp[-1].major,&yymsp[-2].minor.yy382,&yymsp[0].minor.yy382);}
#line 2924 "parse.c"
        break;
      case 164: /* likeop ::= LIKE_KW|MATCH */
#line 989 "parse.y"
{yymsp[0].minor.yy0=yymsp[0].minor.yy0;/*A-overwrites-X*/}
#line 2929 "parse.c"
        break;
      case 165: /* likeop ::= NOT LIKE_KW|MATCH */
#line 990 "parse.y"
{yymsp[-1].minor.yy0=yymsp[0].minor.yy0; yymsp[-1].minor.yy0.n|=0x80000000; /*yymsp[-1].minor.yy0-overwrite-yymsp[0].minor.yy0*/}
#line 2934 "parse.c"
        break;
      case 166: /* expr ::= expr likeop expr */
#line 991 "parse.y"
{
  ExprList *pList;
  int bNot = yymsp[-1].minor.yy0.n & 0x80000000;
  yymsp[-1].minor.yy0.n &= 0x7fffffff;
  pList = sqlite3ExprListAppend(pParse,0, yymsp[0].minor.yy382.pExpr);
  pList = sqlite3ExprListAppend(pParse,pList, yymsp[-2].minor.yy382.pExpr);
  yymsp[-2].minor.yy382.pExpr = sqlite3ExprFunction(pParse, pList, &yymsp[-1].minor.yy0);
  exprNot(pParse, bNot, &yymsp[-2].minor.yy382);
  yymsp[-2].minor.yy382.zEnd = yymsp[0].minor.yy382.zEnd;
  if( yymsp[-2].minor.yy382.pExpr ) yymsp[-2].minor.yy382.pExpr->flags |= EP_InfixFunc;
}
#line 2949 "parse.c"
        break;
      case 167: /* expr ::= expr likeop expr ESCAPE expr */
#line 1002 "parse.y"
{
  ExprList *pList;
  int bNot = yymsp[-3].minor.yy0.n & 0x80000000;
  yymsp[-3].minor.yy0.n &= 0x7fffffff;
  pList = sqlite3ExprListAppend(pParse,0, yymsp[-2].minor.yy382.pExpr);
  pList = sqlite3ExprListAppend(pParse,pList, yymsp[-4].minor.yy382.pExpr);
  pList = sqlite3ExprListAppend(pParse,pList, yymsp[0].minor.yy382.pExpr);
  yymsp[-4].minor.yy382.pExpr = sqlite3ExprFunction(pParse, pList, &yymsp[-3].minor.yy0);
  exprNot(pParse, bNot, &yymsp[-4].minor.yy382);
  yymsp[-4].minor.yy382.zEnd = yymsp[0].minor.yy382.zEnd;
  if( yymsp[-4].minor.yy382.pExpr ) yymsp[-4].minor.yy382.pExpr->flags |= EP_InfixFunc;
}
#line 2965 "parse.c"
        break;
      case 168: /* expr ::= expr ISNULL|NOTNULL */
#line 1029 "parse.y"
{spanUnaryPostfix(pParse,yymsp[0].major,&yymsp[-1].minor.yy382,&yymsp[0].minor.yy0);}
#line 2970 "parse.c"
        break;
      case 169: /* expr ::= expr NOT NULL */
#line 1030 "parse.y"
{spanUnaryPostfix(pParse,TK_NOTNULL,&yymsp[-2].minor.yy382,&yymsp[0].minor.yy0);}
#line 2975 "parse.c"
        break;
      case 170: /* expr ::= expr IS expr */
#line 1051 "parse.y"
{
  spanBinaryExpr(pParse,TK_IS,&yymsp[-2].minor.yy382,&yymsp[0].minor.yy382);
  binaryToUnaryIfNull(pParse, yymsp[0].minor.yy382.pExpr, yymsp[-2].minor.yy382.pExpr, TK_ISNULL);
}
#line 2983 "parse.c"
        break;
      case 171: /* expr ::= expr IS NOT expr */
#line 1055 "parse.y"
{
  spanBinaryExpr(pParse,TK_ISNOT,&yymsp[-3].minor.yy382,&yymsp[0].minor.yy382);
  binaryToUnaryIfNull(pParse, yymsp[0].minor.yy382.pExpr, yymsp[-3].minor.yy382.pExpr, TK_NOTNULL);
}
#line 2991 "parse.c"
        break;
      case 172: /* expr ::= NOT expr */
      case 173: /* expr ::= BITNOT expr */ yytestcase(yyruleno==173);
#line 1079 "parse.y"
{spanUnaryPrefix(&yymsp[-1].minor.yy382,pParse,yymsp[-1].major,&yymsp[0].minor.yy382,&yymsp[-1].minor.yy0);/*A-overwrites-B*/}
#line 2997 "parse.c"
        break;
      case 174: /* expr ::= MINUS expr */
#line 1083 "parse.y"
{spanUnaryPrefix(&yymsp[-1].minor.yy382,pParse,TK_UMINUS,&yymsp[0].minor.yy382,&yymsp[-1].minor.yy0);/*A-overwrites-B*/}
#line 3002 "parse.c"
        break;
      case 175: /* expr ::= PLUS expr */
#line 1085 "parse.y"
{spanUnaryPrefix(&yymsp[-1].minor.yy382,pParse,TK_UPLUS,&yymsp[0].minor.yy382,&yymsp[-1].minor.yy0);/*A-overwrites-B*/}
#line 3007 "parse.c"
        break;
      case 176: /* between_op ::= BETWEEN */
      case 179: /* in_op ::= IN */ yytestcase(yyruleno==179);
#line 1088 "parse.y"
{yymsp[0].minor.yy388 = 0;}
#line 3013 "parse.c"
        break;
      case 178: /* expr ::= expr between_op expr AND expr */
#line 1090 "parse.y"
{
  ExprList *pList = sqlite3ExprListAppend(pParse,0, yymsp[-2].minor.yy382.pExpr);
  pList = sqlite3ExprListAppend(pParse,pList, yymsp[0].minor.yy382.pExpr);
  yymsp[-4].minor.yy382.pExpr = sqlite3PExpr(pParse, TK_BETWEEN, yymsp[-4].minor.yy382.pExpr, 0, 0);
  if( yymsp[-4].minor.yy382.pExpr ){
    yymsp[-4].minor.yy382.pExpr->x.pList = pList;
  }else{
    sqlite3ExprListDelete(pParse->db, pList);
  } 
  exprNot(pParse, yymsp[-3].minor.yy388, &yymsp[-4].minor.yy382);
  yymsp[-4].minor.yy382.zEnd = yymsp[0].minor.yy382.zEnd;
}
#line 3029 "parse.c"
        break;
      case 181: /* expr ::= expr in_op LP exprlist RP */
#line 1106 "parse.y"
{
    if( yymsp[-1].minor.yy250==0 ){
      /* Expressions of the form
      **
      **      expr1 IN ()
      **      expr1 NOT IN ()
      **
      ** simplify to constants 0 (false) and 1 (true), respectively,
      ** regardless of the value of expr1.
      */
      sqlite3ExprDelete(pParse->db, yymsp[-4].minor.yy382.pExpr);
      yymsp[-4].minor.yy382.pExpr = sqlite3PExpr(pParse, TK_INTEGER, 0, 0, &sqlite3IntTokens[yymsp[-3].minor.yy388]);
    }else if( yymsp[-1].minor.yy250->nExpr==1 ){
      /* Expressions of the form:
      **
      **      expr1 IN (?1)
      **      expr1 NOT IN (?2)
      **
      ** with exactly one value on the RHS can be simplified to something
      ** like this:
      **
      **      expr1 == ?1
      **      expr1 <> ?2
      **
      ** But, the RHS of the == or <> is marked with the EP_Generic flag
      ** so that it may not contribute to the computation of comparison
      ** affinity or the collating sequence to use for comparison.  Otherwise,
      ** the semantics would be subtly different from IN or NOT IN.
      */
      Expr *pRHS = yymsp[-1].minor.yy250->a[0].pExpr;
      yymsp[-1].minor.yy250->a[0].pExpr = 0;
      sqlite3ExprListDelete(pParse->db, yymsp[-1].minor.yy250);
      /* pRHS cannot be NULL because a malloc error would have been detected
      ** before now and control would have never reached this point */
      if( ALWAYS(pRHS) ){
        pRHS->flags &= ~EP_Collate;
        pRHS->flags |= EP_Generic;
      }
      yymsp[-4].minor.yy382.pExpr = sqlite3PExpr(pParse, yymsp[-3].minor.yy388 ? TK_NE : TK_EQ, yymsp[-4].minor.yy382.pExpr, pRHS, 0);
    }else{
      yymsp[-4].minor.yy382.pExpr = sqlite3PExpr(pParse, TK_IN, yymsp[-4].minor.yy382.pExpr, 0, 0);
      if( yymsp[-4].minor.yy382.pExpr ){
        yymsp[-4].minor.yy382.pExpr->x.pList = yymsp[-1].minor.yy250;
        sqlite3ExprSetHeightAndFlags(pParse, yymsp[-4].minor.yy382.pExpr);
      }else{
        sqlite3ExprListDelete(pParse->db, yymsp[-1].minor.yy250);
      }
      exprNot(pParse, yymsp[-3].minor.yy388, &yymsp[-4].minor.yy382);
    }
    yymsp[-4].minor.yy382.zEnd = &yymsp[0].minor.yy0.z[yymsp[0].minor.yy0.n];
  }
#line 3084 "parse.c"
        break;
      case 182: /* expr ::= LP select RP */
#line 1157 "parse.y"
{
    spanSet(&yymsp[-2].minor.yy382,&yymsp[-2].minor.yy0,&yymsp[0].minor.yy0); /*A-overwrites-B*/
    yymsp[-2].minor.yy382.pExpr = sqlite3PExpr(pParse, TK_SELECT, 0, 0, 0);
    sqlite3PExprAddSelect(pParse, yymsp[-2].minor.yy382.pExpr, yymsp[-1].minor.yy219);
  }
#line 3093 "parse.c"
        break;
      case 183: /* expr ::= expr in_op LP select RP */
#line 1162 "parse.y"
{
    yymsp[-4].minor.yy382.pExpr = sqlite3PExpr(pParse, TK_IN, yymsp[-4].minor.yy382.pExpr, 0, 0);
    sqlite3PExprAddSelect(pParse, yymsp[-4].minor.yy382.pExpr, yymsp[-1].minor.yy219);
    exprNot(pParse, yymsp[-3].minor.yy388, &yymsp[-4].minor.yy382);
    yymsp[-4].minor.yy382.zEnd = &yymsp[0].minor.yy0.z[yymsp[0].minor.yy0.n];
  }
#line 3103 "parse.c"
        break;
      case 184: /* expr ::= expr in_op nm dbnm paren_exprlist */
#line 1168 "parse.y"
{
    SrcList *pSrc = sqlite3SrcListAppend(pParse->db, 0,&yymsp[-2].minor.yy0,&yymsp[-1].minor.yy0);
    Select *pSelect = sqlite3SelectNew(pParse, 0,pSrc,0,0,0,0,0,0,0);
    if( yymsp[0].minor.yy250 )  sqlite3SrcListFuncArgs(pParse, pSelect ? pSrc : 0, yymsp[0].minor.yy250);
    yymsp[-4].minor.yy382.pExpr = sqlite3PExpr(pParse, TK_IN, yymsp[-4].minor.yy382.pExpr, 0, 0);
    sqlite3PExprAddSelect(pParse, yymsp[-4].minor.yy382.pExpr, pSelect);
    exprNot(pParse, yymsp[-3].minor.yy388, &yymsp[-4].minor.yy382);
    yymsp[-4].minor.yy382.zEnd = yymsp[-1].minor.yy0.z ? &yymsp[-1].minor.yy0.z[yymsp[-1].minor.yy0.n] : &yymsp[-2].minor.yy0.z[yymsp[-2].minor.yy0.n];
  }
#line 3116 "parse.c"
        break;
      case 185: /* expr ::= EXISTS LP select RP */
#line 1177 "parse.y"
{
    Expr *p;
    spanSet(&yymsp[-3].minor.yy382,&yymsp[-3].minor.yy0,&yymsp[0].minor.yy0); /*A-overwrites-B*/
    p = yymsp[-3].minor.yy382.pExpr = sqlite3PExpr(pParse, TK_EXISTS, 0, 0, 0);
    sqlite3PExprAddSelect(pParse, p, yymsp[-1].minor.yy219);
  }
#line 3126 "parse.c"
        break;
      case 186: /* expr ::= CASE case_operand case_exprlist case_else END */
#line 1186 "parse.y"
{
  spanSet(&yymsp[-4].minor.yy382,&yymsp[-4].minor.yy0,&yymsp[0].minor.yy0);  /*A-overwrites-C*/
  yymsp[-4].minor.yy382.pExpr = sqlite3PExpr(pParse, TK_CASE, yymsp[-3].minor.yy362, 0, 0);
  if( yymsp[-4].minor.yy382.pExpr ){
    yymsp[-4].minor.yy382.pExpr->x.pList = yymsp[-1].minor.yy362 ? sqlite3ExprListAppend(pParse,yymsp[-2].minor.yy250,yymsp[-1].minor.yy362) : yymsp[-2].minor.yy250;
    sqlite3ExprSetHeightAndFlags(pParse, yymsp[-4].minor.yy382.pExpr);
  }else{
    sqlite3ExprListDelete(pParse->db, yymsp[-2].minor.yy250);
    sqlite3ExprDelete(pParse->db, yymsp[-1].minor.yy362);
  }
}
#line 3141 "parse.c"
        break;
      case 187: /* case_exprlist ::= case_exprlist WHEN expr THEN expr */
#line 1199 "parse.y"
{
  yymsp[-4].minor.yy250 = sqlite3ExprListAppend(pParse,yymsp[-4].minor.yy250, yymsp[-2].minor.yy382.pExpr);
  yymsp[-4].minor.yy250 = sqlite3ExprListAppend(pParse,yymsp[-4].minor.yy250, yymsp[0].minor.yy382.pExpr);
}
#line 3149 "parse.c"
        break;
      case 188: /* case_exprlist ::= WHEN expr THEN expr */
#line 1203 "parse.y"
{
  yymsp[-3].minor.yy250 = sqlite3ExprListAppend(pParse,0, yymsp[-2].minor.yy382.pExpr);
  yymsp[-3].minor.yy250 = sqlite3ExprListAppend(pParse,yymsp[-3].minor.yy250, yymsp[0].minor.yy382.pExpr);
}
#line 3157 "parse.c"
        break;
      case 191: /* case_operand ::= expr */
#line 1213 "parse.y"
{yymsp[0].minor.yy362 = yymsp[0].minor.yy382.pExpr; /*A-overwrites-X*/}
#line 3162 "parse.c"
        break;
      case 194: /* nexprlist ::= nexprlist COMMA expr */
#line 1224 "parse.y"
{yymsp[-2].minor.yy250 = sqlite3ExprListAppend(pParse,yymsp[-2].minor.yy250,yymsp[0].minor.yy382.pExpr);}
#line 3167 "parse.c"
        break;
      case 195: /* nexprlist ::= expr */
#line 1226 "parse.y"
{yymsp[0].minor.yy250 = sqlite3ExprListAppend(pParse,0,yymsp[0].minor.yy382.pExpr); /*A-overwrites-Y*/}
#line 3172 "parse.c"
        break;
      case 197: /* paren_exprlist ::= LP exprlist RP */
      case 202: /* eidlist_opt ::= LP eidlist RP */ yytestcase(yyruleno==202);
#line 1234 "parse.y"
{yymsp[-2].minor.yy250 = yymsp[-1].minor.yy250;}
#line 3178 "parse.c"
        break;
      case 198: /* cmd ::= createkw uniqueflag INDEX ifnotexists nm dbnm ON nm LP sortlist RP where_opt */
#line 1241 "parse.y"
{
  sqlite3CreateIndex(pParse, &yymsp[-7].minor.yy0, &yymsp[-6].minor.yy0, 
                     sqlite3SrcListAppend(pParse->db,0,&yymsp[-4].minor.yy0,0), yymsp[-2].minor.yy250, yymsp[-10].minor.yy388,
                      &yymsp[-11].minor.yy0, yymsp[0].minor.yy362, SQLITE_SO_ASC, yymsp[-8].minor.yy388, SQLITE_IDXTYPE_APPDEF);
}
#line 3187 "parse.c"
        break;
      case 199: /* uniqueflag ::= UNIQUE */
      case 211: /* raisetype ::= ABORT */ yytestcase(yyruleno==211);
#line 1248 "parse.y"
{yymsp[0].minor.yy388 = OE_Abort;}
#line 3193 "parse.c"
        break;
      case 200: /* uniqueflag ::= */
#line 1249 "parse.y"
{yymsp[1].minor.yy388 = OE_None;}
#line 3198 "parse.c"
        break;
      case 203: /* eidlist ::= eidlist COMMA nm collate sortorder */
#line 1299 "parse.y"
{
  yymsp[-4].minor.yy250 = parserAddExprIdListTerm(pParse, yymsp[-4].minor.yy250, &yymsp[-2].minor.yy0, yymsp[-1].minor.yy388, yymsp[0].minor.yy388);
}
#line 3205 "parse.c"
        break;
      case 204: /* eidlist ::= nm collate sortorder */
#line 1302 "parse.y"
{
  yymsp[-2].minor.yy250 = parserAddExprIdListTerm(pParse, 0, &yymsp[-2].minor.yy0, yymsp[-1].minor.yy388, yymsp[0].minor.yy388); /*A-overwrites-Y*/
}
#line 3212 "parse.c"
        break;
      case 207: /* cmd ::= DROP INDEX ifexists fullname */
#line 1313 "parse.y"
{sqlite3DropIndex(pParse, yymsp[0].minor.yy259, yymsp[-1].minor.yy388);}
#line 3217 "parse.c"
        break;
      case 210: /* raisetype ::= ROLLBACK */
#line 1463 "parse.y"
{yymsp[0].minor.yy388 = OE_Rollback;}
#line 3222 "parse.c"
        break;
      case 212: /* raisetype ::= FAIL */
#line 1465 "parse.y"
{yymsp[0].minor.yy388 = OE_Fail;}
#line 3227 "parse.c"
        break;
      case 213: /* with ::= */
#line 1550 "parse.y"
{yymsp[1].minor.yy163 = 0;}
#line 3232 "parse.c"
        break;
      default:
      /* (214) input ::= cmdlist */ yytestcase(yyruleno==214);
      /* (215) cmdlist ::= cmdlist ecmd */ yytestcase(yyruleno==215);
      /* (216) cmdlist ::= ecmd (OPTIMIZED OUT) */ assert(yyruleno!=216);
      /* (217) ecmd ::= SEMI */ yytestcase(yyruleno==217);
      /* (218) ecmd ::= explain cmdx SEMI */ yytestcase(yyruleno==218);
      /* (219) explain ::= */ yytestcase(yyruleno==219);
      /* (220) trans_opt ::= */ yytestcase(yyruleno==220);
      /* (221) trans_opt ::= TRANSACTION */ yytestcase(yyruleno==221);
      /* (222) trans_opt ::= TRANSACTION nm */ yytestcase(yyruleno==222);
      /* (223) savepoint_opt ::= SAVEPOINT */ yytestcase(yyruleno==223);
      /* (224) savepoint_opt ::= */ yytestcase(yyruleno==224);
      /* (225) cmd ::= create_table create_table_args */ yytestcase(yyruleno==225);
      /* (226) columnlist ::= columnlist COMMA columnname carglist */ yytestcase(yyruleno==226);
      /* (227) columnlist ::= columnname carglist */ yytestcase(yyruleno==227);
      /* (228) nm ::= ID|INDEXED */ yytestcase(yyruleno==228);
      /* (229) nm ::= STRING */ yytestcase(yyruleno==229);
      /* (230) nm ::= JOIN_KW */ yytestcase(yyruleno==230);
      /* (231) typetoken ::= typename */ yytestcase(yyruleno==231);
      /* (232) typename ::= ID|STRING */ yytestcase(yyruleno==232);
      /* (233) signed ::= plus_num (OPTIMIZED OUT) */ assert(yyruleno!=233);
      /* (234) signed ::= minus_num (OPTIMIZED OUT) */ assert(yyruleno!=234);
      /* (235) carglist ::= carglist ccons */ yytestcase(yyruleno==235);
      /* (236) carglist ::= */ yytestcase(yyruleno==236);
      /* (237) ccons ::= NULL onconf */ yytestcase(yyruleno==237);
      /* (238) conslist_opt ::= COMMA conslist */ yytestcase(yyruleno==238);
      /* (239) conslist ::= conslist tconscomma tcons */ yytestcase(yyruleno==239);
      /* (240) conslist ::= tcons (OPTIMIZED OUT) */ assert(yyruleno!=240);
      /* (241) tconscomma ::= */ yytestcase(yyruleno==241);
      /* (242) defer_subclause_opt ::= defer_subclause (OPTIMIZED OUT) */ assert(yyruleno!=242);
      /* (243) resolvetype ::= raisetype (OPTIMIZED OUT) */ assert(yyruleno!=243);
      /* (244) selectnowith ::= oneselect (OPTIMIZED OUT) */ assert(yyruleno!=244);
      /* (245) oneselect ::= values */ yytestcase(yyruleno==245);
      /* (246) sclp ::= selcollist COMMA */ yytestcase(yyruleno==246);
      /* (247) as ::= ID|STRING */ yytestcase(yyruleno==247);
      /* (248) expr ::= term (OPTIMIZED OUT) */ assert(yyruleno!=248);
      /* (249) exprlist ::= nexprlist */ yytestcase(yyruleno==249);
      /* (250) plus_num ::= INTEGER|FLOAT */ yytestcase(yyruleno==250);
        break;
/********** End reduce actions ************************************************/
  };
  assert( yyruleno<sizeof(yyRuleInfo)/sizeof(yyRuleInfo[0]) );
  yygoto = yyRuleInfo[yyruleno].lhs;
  yysize = yyRuleInfo[yyruleno].nrhs;
  yyact = yy_find_reduce_action(yymsp[-yysize].stateno,(YYCODETYPE)yygoto);
  if( yyact <= YY_MAX_SHIFTREDUCE ){
    if( yyact>YY_MAX_SHIFT ){
      yyact += YY_MIN_REDUCE - YY_MIN_SHIFTREDUCE;
    }
    yymsp -= yysize-1;
    yypParser->yytos = yymsp;
    yymsp->stateno = (YYACTIONTYPE)yyact;
    yymsp->major = (YYCODETYPE)yygoto;
    yyTraceShift(yypParser, yyact);
  }else{
    assert( yyact == YY_ACCEPT_ACTION );
    yypParser->yytos -= yysize;
    yy_accept(yypParser);
  }
}

/*
** The following code executes when the parse fails
*/
#ifndef YYNOERRORRECOVERY
static void yy_parse_failed(
  yyParser *yypParser           /* The parser */
){
  sqlite3ParserARG_FETCH;
#ifndef NDEBUG
  if( yyTraceFILE ){
    fprintf(yyTraceFILE,"%sFail!\n",yyTracePrompt);
  }
#endif
  while( yypParser->yytos>yypParser->yystack ) yy_pop_parser_stack(yypParser);
  /* Here code is inserted which will be executed whenever the
  ** parser fails */
/************ Begin %parse_failure code ***************************************/
/************ End %parse_failure code *****************************************/
  sqlite3ParserARG_STORE; /* Suppress warning about unused %extra_argument variable */
}
#endif /* YYNOERRORRECOVERY */

/*
** The following code executes when a syntax error first occurs.
*/
static void yy_syntax_error(
  yyParser *yypParser,           /* The parser */
  int yymajor,                   /* The major type of the error token */
  sqlite3ParserTOKENTYPE yyminor         /* The minor type of the error token */
){
  sqlite3ParserARG_FETCH;
#define TOKEN yyminor
/************ Begin %syntax_error code ****************************************/
#line 32 "parse.y"

  UNUSED_PARAMETER(yymajor);  /* Silence some compiler warnings */
  assert( TOKEN.z[0] );  /* The tokenizer always gives us a token */
  sqlite3ErrorMsg(pParse, "near \"%T\": syntax error", &TOKEN);
#line 3333 "parse.c"
/************ End %syntax_error code ******************************************/
  sqlite3ParserARG_STORE; /* Suppress warning about unused %extra_argument variable */
}

/*
** The following is executed when the parser accepts
*/
static void yy_accept(
  yyParser *yypParser           /* The parser */
){
  sqlite3ParserARG_FETCH;
#ifndef NDEBUG
  if( yyTraceFILE ){
    fprintf(yyTraceFILE,"%sAccept!\n",yyTracePrompt);
  }
#endif
#ifndef YYNOERRORRECOVERY
  yypParser->yyerrcnt = -1;
#endif
  assert( yypParser->yytos==yypParser->yystack );
  /* Here code is inserted which will be executed whenever the
  ** parser accepts */
/*********** Begin %parse_accept code *****************************************/
/*********** End %parse_accept code *******************************************/
  sqlite3ParserARG_STORE; /* Suppress warning about unused %extra_argument variable */
}

/* The main parser program.
** The first argument is a pointer to a structure obtained from
** "sqlite3ParserAlloc" which describes the current state of the parser.
** The second argument is the major token number.  The third is
** the minor token.  The fourth optional argument is whatever the
** user wants (and specified in the grammar) and is available for
** use by the action routines.
**
** Inputs:
** <ul>
** <li> A pointer to the parser (an opaque structure.)
** <li> The major token number.
** <li> The minor token number.
** <li> An option argument of a grammar-specified type.
** </ul>
**
** Outputs:
** None.
*/
void sqlite3Parser(
  void *yyp,                   /* The parser */
  int yymajor,                 /* The major token code number */
  sqlite3ParserTOKENTYPE yyminor       /* The value for the token */
  sqlite3ParserARG_PDECL               /* Optional %extra_argument parameter */
){
  YYMINORTYPE yyminorunion;
  unsigned int yyact;   /* The parser action. */
#if !defined(YYERRORSYMBOL) && !defined(YYNOERRORRECOVERY)
  int yyendofinput;     /* True if we are at the end of input */
#endif
#ifdef YYERRORSYMBOL
  int yyerrorhit = 0;   /* True if yymajor has invoked an error */
#endif
  yyParser *yypParser;  /* The parser */

  yypParser = (yyParser*)yyp;
  assert( yypParser->yytos!=0 );
#if !defined(YYERRORSYMBOL) && !defined(YYNOERRORRECOVERY)
  yyendofinput = (yymajor==0);
#endif
  sqlite3ParserARG_STORE;

#ifndef NDEBUG
  if( yyTraceFILE ){
    fprintf(yyTraceFILE,"%sInput '%s'\n",yyTracePrompt,yyTokenName[yymajor]);
  }
#endif

  do{
    yyact = yy_find_shift_action(yypParser,(YYCODETYPE)yymajor);
    if( yyact <= YY_MAX_SHIFTREDUCE ){
      yy_shift(yypParser,yyact,yymajor,yyminor);
#ifndef YYNOERRORRECOVERY
      yypParser->yyerrcnt--;
#endif
      yymajor = YYNOCODE;
    }else if( yyact <= YY_MAX_REDUCE ){
      yy_reduce(yypParser,yyact-YY_MIN_REDUCE);
    }else{
      assert( yyact == YY_ERROR_ACTION );
      yyminorunion.yy0 = yyminor;
#ifdef YYERRORSYMBOL
      int yymx;
#endif
#ifndef NDEBUG
      if( yyTraceFILE ){
        fprintf(yyTraceFILE,"%sSyntax Error!\n",yyTracePrompt);
      }
#endif
#ifdef YYERRORSYMBOL
      /* A syntax error has occurred.
      ** The response to an error depends upon whether or not the
      ** grammar defines an error token "ERROR".  
      **
      ** This is what we do if the grammar does define ERROR:
      **
      **  * Call the %syntax_error function.
      **
      **  * Begin popping the stack until we enter a state where
      **    it is legal to shift the error symbol, then shift
      **    the error symbol.
      **
      **  * Set the error count to three.
      **
      **  * Begin accepting and shifting new tokens.  No new error
      **    processing will occur until three tokens have been
      **    shifted successfully.
      **
      */
      if( yypParser->yyerrcnt<0 ){
        yy_syntax_error(yypParser,yymajor,yyminor);
      }
      yymx = yypParser->yytos->major;
      if( yymx==YYERRORSYMBOL || yyerrorhit ){
#ifndef NDEBUG
        if( yyTraceFILE ){
          fprintf(yyTraceFILE,"%sDiscard input token %s\n",
             yyTracePrompt,yyTokenName[yymajor]);
        }
#endif
        yy_destructor(yypParser, (YYCODETYPE)yymajor, &yyminorunion);
        yymajor = YYNOCODE;
      }else{
        while( yypParser->yytos >= yypParser->yystack
            && yymx != YYERRORSYMBOL
            && (yyact = yy_find_reduce_action(
                        yypParser->yytos->stateno,
                        YYERRORSYMBOL)) >= YY_MIN_REDUCE
        ){
          yy_pop_parser_stack(yypParser);
        }
        if( yypParser->yytos < yypParser->yystack || yymajor==0 ){
          yy_destructor(yypParser,(YYCODETYPE)yymajor,&yyminorunion);
          yy_parse_failed(yypParser);
#ifndef YYNOERRORRECOVERY
          yypParser->yyerrcnt = -1;
#endif
          yymajor = YYNOCODE;
        }else if( yymx!=YYERRORSYMBOL ){
          yy_shift(yypParser,yyact,YYERRORSYMBOL,yyminor);
        }
      }
      yypParser->yyerrcnt = 3;
      yyerrorhit = 1;
#elif defined(YYNOERRORRECOVERY)
      /* If the YYNOERRORRECOVERY macro is defined, then do not attempt to
      ** do any kind of error recovery.  Instead, simply invoke the syntax
      ** error routine and continue going as if nothing had happened.
      **
      ** Applications can set this macro (for example inside %include) if
      ** they intend to abandon the parse upon the first syntax error seen.
      */
      yy_syntax_error(yypParser,yymajor, yyminor);
      yy_destructor(yypParser,(YYCODETYPE)yymajor,&yyminorunion);
      yymajor = YYNOCODE;
      
#else  /* YYERRORSYMBOL is not defined */
      /* This is what we do if the grammar does not define ERROR:
      **
      **  * Report an error message, and throw away the input token.
      **
      **  * If the input token is $, then fail the parse.
      **
      ** As before, subsequent error messages are suppressed until
      ** three input tokens have been successfully shifted.
      */
      if( yypParser->yyerrcnt<=0 ){
        yy_syntax_error(yypParser,yymajor, yyminor);
      }
      yypParser->yyerrcnt = 3;
      yy_destructor(yypParser,(YYCODETYPE)yymajor,&yyminorunion);
      if( yyendofinput ){
        yy_parse_failed(yypParser);
#ifndef YYNOERRORRECOVERY
        yypParser->yyerrcnt = -1;
#endif
      }
      yymajor = YYNOCODE;
#endif
    }
  }while( yymajor!=YYNOCODE && yypParser->yytos>yypParser->yystack );
#ifndef NDEBUG
  if( yyTraceFILE ){
    yyStackEntry *i;
    char cDiv = '[';
    fprintf(yyTraceFILE,"%sReturn. Stack=",yyTracePrompt);
    for(i=&yypParser->yystack[1]; i<=yypParser->yytos; i++){
      fprintf(yyTraceFILE,"%c%s", cDiv, yyTokenName[i->major]);
      cDiv = ' ';
    }
    fprintf(yyTraceFILE,"]\n");
  }
#endif
  return;
}
