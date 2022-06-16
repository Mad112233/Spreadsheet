#include "formula.h"
#include "FormulaAST.h"

#include <algorithm>
#include <cassert>
#include <cctype>
#include <sstream>

using namespace std::literals;

std::ostream& operator<<(std::ostream& output, FormulaError fe) {
    return output << "#DIV/0!";
}

namespace {
    class Formula : public FormulaInterface {
    public:
        // Реализуйте следующие методы:
        explicit Formula(std::string expression) : ast_(ParseFormulaAST(expression)) {}

        Value Evaluate(const SheetInterface& sheet) const override {
            CellLook cell_look = [&sheet](Position pos) {
                double ans = 0.0;

                if (!pos.IsValid())
                    throw FormulaError(FormulaError::Category::Ref);

                if (sheet.GetCell(pos) == nullptr)
                    return ans;

                CellInterface::Value value = sheet.GetCell(pos)->GetValue();
                
                if (value.index() == 1)
                    ans = std::get<1>(value);
                else if (value.index() == 2)
                    throw std::get<2>(value);
                else if (value.index() == 0) {
                    try {
                        ans = std::stod(std::get<0>(value));
                    }
                    catch (std::invalid_argument& err) {
                        throw FormulaError(FormulaError::Category::Value);
                    }
                }

                return ans;
            };

            Value value;
            try {
                value = ast_.Execute(cell_look);
            }
            catch (FormulaError& error) {
                value = error;
            }

            return value;
        }

        std::string GetExpression() const override {
            std::stringstream oss;
            ast_.PrintFormula(oss);
            
            return oss.str();
        }

        std::vector<Position> GetReferencedCells() const {
            std::vector<Position> arr;

            for (const auto& cell : ast_.GetCells()) {
                arr.push_back(cell);
            }

            return arr;
        }

    private:
        FormulaAST ast_;
    };
}  // namespace

std::unique_ptr<FormulaInterface> ParseFormula(std::string expression) {
    return std::make_unique<Formula>(std::move(expression));
}