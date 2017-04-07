#include "pch.h"
#include "VariableTable.h"

namespace xo {

VariableTable::VariableTable(xo::Doc* doc) : Doc(doc) {
}

VariableTable::~VariableTable() {
}

int VariableTable::Set(const char* var, const char* value, size_t valueMaxLen) {
	if (var[0] == 0)
		return 0;
	int id = IDTable.GetOrCreateID(var);
	while ((size_t) id >= Values.size()) {
		Values.push(String());
		IsModified.push(true);
	}
	Values[id].Set(value, valueMaxLen);
	IsModified[id] = true;
	return id;
}

const char* VariableTable::GetByName(const char* var) const {
	int id = IDTable.GetID(var);
	if (id == 0)
		return nullptr;
	return Values[id].Z;
}

const char* VariableTable::GetByID(int id) const {
	if ((size_t) id >= Values.size())
		return nullptr;
	return Values[id].Z;
}

int VariableTable::GetID(const char* var) const {
	return IDTable.GetID(var);
}

void VariableTable::CloneFrom_Incremental(const VariableTable& src) {
	IDTable.CloneFrom_Incremental(src.IDTable);

	// copy new
	size_t orgSize = Values.size();
	for (size_t i = orgSize; i < src.Values.size(); i++)
		Values.push(src.Values[i]);

	// copy changed
	for (size_t i = 0; i < orgSize; i++) {
		if (src.IsModified[i])
			Values[i] = src.Values[i];
	}

	// modified bits are cleared by Doc::ResetModified()
}

void VariableTable::ResetModified() {
	//IDTable.ResetModified();
	IsModified.fill(false);
}
}