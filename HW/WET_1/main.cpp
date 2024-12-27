#include "tokens.hpp"
#include "output.hpp"
#include "iostream"
#include <sstream>
#include <cstring>

using namespace std;
char getSequence(char* yytext);
void print_string(char *yytext , int lineno, enum tokentype token );
char* charToCharPtr(char c);
string getUndefHexSequence(char* yytext);

int main() {
    enum tokentype token;
    // read tokens until the end of file is reached
    while ((token = static_cast<tokentype>(yylex()))) {
        // your code here
        if (token == ERROR) {
			output::errorUnknownChar(*yytext);
            exit(0);
        }
        if (token == ERROR_UNCLOSED_STRING) {
			output::errorUnclosedString();
            exit(0);
        }
        if (token == ERROR_UNDEF_ESCAPE) {
            //printf("ERROR_UNDEF_ESCAPE");
            char sequence = getSequence(yytext);
            char* dequence_p = charToCharPtr(sequence);
			output::errorUndefinedEscape(dequence_p);
            exit(0);
        }
        if(token == ERROR_UNDEF_HEX) {
            //printf("ERROR_UNDEF_hex");
            string sequence = getUndefHexSequence(yytext); 
            output::errorUndefinedEscape(sequence.c_str());
            exit(0);
        }
        if (token == STRING) {
            //printf("string");
			print_string(yytext , yylineno , token);
            continue;
        }
		output::printToken(yylineno, token, yytext);
    }
    return 0;
}


//this is for valid string
void print_string(char *yytext , int lineno, enum tokentype token )
{
    string str(yytext);
    //cout << yylineno << " " << "STRING" << " ";
    std::cout << lineno << " " << "STRING" << " ";
    for(int i = 1; i < str.size(); i++) {
        char curr = str[i];
        char next = str[i+1];
        if(curr == '\"') break;
        else if(curr == '\\'){
            if(next == 'n'){
                cout << '\n';
                i++;
            } else if(next == 'r') {
                cout << '\r';
                i++;
            } else if(next == '0') {
                break;
            } else if(next == 't') {
                cout << '\t';
                i++;
            } else if(next == '\\') {
                cout << '\\';
                i++;
            } else if(next == '\"') {
                cout << '\"';
                i++;
            } else if(next == 'x') {//hex
                stringstream ss;
                int val;
                ss << std::hex << str.substr(i+2,2);
                ss >> val;
                cout << char(val);
                i+=3;
            } 
        } 
        else {
            cout << curr;
        }  
    }
    cout << endl;
}

char getSequence(char* yytext)
{
    for( int i=0 ; i< strlen(yytext)-1 ; i++)
    {
        char cur_char = yytext[i];
        char next_char = yytext[i+1];

        if(cur_char == '\\')
        {
            if( next_char != 't' && next_char != 'r' && next_char != 'n' && next_char != '\\' &&next_char != '"' &&next_char != 'x')
            {
                return next_char;
            }
        }
        

    }
    return '\\';
}

bool isValidHex(char first , char second) {
    return (first >= '2' && first <= '7') && ((second >= '0' && second <= '9') || (second >= 'A' && second <= 'E') ||(second >= 'a' && second <= 'e'));
}


//

string getUndefHexSequence(char* yytext) {
    int length = strlen(yytext);
    for (int i = 1; i < length - 1; i++) { //
        
        char cur_char = yytext[i];
        char next_char = yytext[i + 1];

        if (cur_char == '\\' && next_char == 'x') {
           
            if(!isValidHex(yytext[i + 2], yytext[i+3])) {
                    if(i + 3>= length || yytext[i + 2] == '\"'){
                        return string("x") ;
                    }
                    if(i + 3>= length || yytext[i + 3] == '\"'){
                        return string("x") + string(1, yytext[i+2]);
                    }
                    return string("x") + string(1, yytext[i + 2]) + string(1, yytext[i + 3]);
                }

        } 
    }
return "";
}



char* charToCharPtr(char c) {
    static char buffer[2]; // Static buffer to ensure it persists after function returns
    buffer[0] = c;         // Store the character
    buffer[1] = '\0';      // Null-terminate the string
    return buffer;
}

