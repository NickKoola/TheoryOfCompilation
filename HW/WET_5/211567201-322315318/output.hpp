#ifndef OUTPUT_HPP
#define OUTPUT_HPP

#include <vector>
#include <string>
#include <sstream>
#include "visitor.hpp"
#include "nodes.hpp"

namespace output {
    /* Error handling functions */

    void errorLex(int lineno);

    void errorSyn(int lineno);

    void errorUndef(int lineno, const std::string &id);

    void errorDefAsFunc(int lineno, const std::string &id);

    void errorUndefFunc(int lineno, const std::string &id);

    void errorDefAsVar(int lineno, const std::string &id);

    void errorDef(int lineno, const std::string &id);

    void errorPrototypeMismatch(int lineno, const std::string &id, std::vector<std::string> &paramTypes);

    void errorMismatch(int lineno);

    void errorUnexpectedBreak(int lineno);

    void errorUnexpectedContinue(int lineno);

    void errorMainMissing();

    void errorByteTooLarge(int lineno, int value);

    /* CodeBuffer class
     * This class is used to store the generated code.
     * It provides a simple interface to emit code and manage labels and variables.
     */
    class CodeBuffer {
    private:
        std::stringstream globalsBuffer;
        std::stringstream buffer;
        int labelCount;
        int varCount;
        int stringCount;
        int argCount;

        friend std::ostream &operator<<(std::ostream &os, const CodeBuffer &buffer);

        std::vector<std::pair<std::string, std::string>> loopLabelStack;

    public:
        CodeBuffer();

        // Returns a string that represents a label not used before
        // Usage examples:
        //      emitLabel(freshLabel());
        //      buffer << "br label " << freshLabel() << std::endl;
        std::string freshLabel();

        // Returns a string that represents a variable not used before
        // Usage examples:
        //      std::string var = freshVar();
        //      buffer << var << " = icmp eq i32 0, 0" << std::endl;
        std::string freshVar();

        // Emits a label into the buffer
        void emitLabel(const std::string &label);

        // Emits a constant string into the globals section of the code.
        // Returns the name of the constant. For the string of the length n (not including null character), the type is [n+1 x i8]
        // Usage examples:
        //      std::string str = emitString("Hello, World!");
        //      buffer << "call i32 (i8*, ...) @printf(i8* getelementptr inbounds ([14 x i8], [14 x i8]* " << str << ", i32 0, i32 0))" << std::endl;
        std::string emitString(const std::string &str);

        // Emits a string into the buffer
        void emit(const std::string &str);

        // Template overload for general types
        template<typename T>
        CodeBuffer &operator<<(const T &value) {
            buffer << value;
            return *this;
        }

        // Overload for manipulators (like std::endl)
        CodeBuffer &operator<<(std::ostream &(*manip)(std::ostream &));


        void pushLoopLabels(const std::string &startLabel, const std::string &endLabel) {
            loopLabelStack.emplace_back(startLabel, endLabel);
        }

        void popLoopLabels() {
            if (!loopLabelStack.empty()) {
                loopLabelStack.pop_back();
            } else {
                throw std::runtime_error("Loop label stack underflow: no active loop labels to pop.");
            }
        }

        std::string getLoopStartLabel() const {
            if (!loopLabelStack.empty()) {
                return loopLabelStack.back().first;
            } else {
                throw std::runtime_error("No active loop start label found.");
            }
        }

        std::string getLoopEndLabel() const {
            if (!loopLabelStack.empty()) {
                return loopLabelStack.back().second;
            } else {
                throw std::runtime_error("No active loop end label found.");
            }
        }

        int getArgCount(){
            argCount++;
            return argCount;
        }

    };

    std::ostream &operator<<(std::ostream &os, const CodeBuffer &buffer);
}

#endif //OUTPUT_HPP
