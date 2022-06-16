#include "cell.h"

#include <cassert>
#include <iostream>
#include <string>
#include <optional>

Cell::Cell(SheetInterface& sheet) :sheet_(sheet) {}
Cell::~Cell() {}

void Cell::Set(const std::string& text) {
	if (text.empty()) {
		Clear();
	}
	else if (text[0] == '=' && text.size() > 1) {
		impl_ = std::make_unique<FormulaImpl>(sheet_, text);
	}
	else {
		impl_ = std::make_unique<TextImpl>(text);
	}
}

void Cell::Clear() {
	impl_ = nullptr;
}

std::vector<Position> Cell::GetReferencedCells() const {
	if (impl_ == nullptr)
		return {};

	return impl_->GetReferencedCells();
}

void Cell::ClearCache() {
	if (impl_ == nullptr)
		return;

	impl_->ClearCache();
}

const std::vector<Position> Cell::GetDependentCells() const {
	return dependent_cells_;
}

CellInterface::Value Cell::GetValue() const {
	if (impl_ == nullptr)
		return {};

	return impl_->GetValue();
}

std::string Cell::GetText() const {
	if (impl_ == nullptr)
		return {};

	return impl_->GetText();
}

void Cell::AddDependentCell(Position pos) {
	dependent_cells_.push_back(pos);
}

TextImpl::TextImpl(const std::string& str) :text_(str) {}

CellInterface::Value TextImpl::GetValue() {
	if (!text_.empty() && text_[0] == '\'')
		return text_.substr(1);

	return text_;
}

std::string TextImpl::GetText() const {
	return text_;
}

std::vector<Position> TextImpl::GetReferencedCells() const {
	return {};
}

void TextImpl::ClearCache() {}

FormulaImpl::FormulaImpl(const SheetInterface& sheet, const std::string& str) :sheet_(sheet) {
	try {
		formula_ptr_ = ParseFormula(str.substr(1));
	}
	catch (...) {
		throw FormulaException("Error in the formula!");
	}
}

CellInterface::Value FormulaImpl::GetValue() {
	if (cache_ == std::nullopt) {
		const auto value = formula_ptr_->Evaluate(sheet_);

		if (value.index() == 0)
			cache_ = std::get<0>(value);
		else
			cache_ = std::get<1>(value);
	}

	return cache_.value();
}

std::string FormulaImpl::GetText() const {
	return '=' + formula_ptr_->GetExpression();
}

void FormulaImpl::ClearCache() {
	cache_ = std::nullopt;
}

std::vector<Position> FormulaImpl::GetReferencedCells() const {
	return formula_ptr_->GetReferencedCells();
}

std::ostream& operator<<(std::ostream& output, const CellInterface::Value& value) {
	std::visit([&](const auto& val) {
		output << val; 
		}, value);

	return output;
}