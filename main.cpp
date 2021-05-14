#include "eeyore_AST.hpp"
#include "node.hpp"
#include "riscv/emit_riscv.hpp"
#include "symtab.hpp"
#include "tigger_gen.hpp"
#include "SysY.tab.hpp"
#include <cstdio>
#include <sstream>
#include <unistd.h>
extern FILE *yyin;
int yyparse(void);
int yylex(void);
namespace sysY_AST {
node_basic *root; // root of the parse tree
}
bool has_err = false; // true if an error occurred
std::ostringstream tigger_dst;
std::ostringstream riscv_dst;
int main(int argc, char **argv)
{
	const char *optstring = "S::ve:t:o:";
	bool S = false, e = false, t = false, v = false;
	string output_s;
	char ch;
	while ((ch = getopt(argc, argv, optstring)) != -1)
	{
		switch (ch)
		{
		case 'S':
			S = true;
			yyin = fopen(optarg, "r");
			break;
		case 'e':
			e = true;
			yyin = fopen(optarg, "r");
			break;
		case 't':
			t = true;
			yyin = fopen(optarg, "r");
			break;
		case 'v': v = true; break;
		case 'o': output_s = optarg; break;
		default: printf("Unknown parameter\n"); return 0;
		}
	}
	if (!S)
	{
		printf("not support yet!\n");
		return 0;
	}
	if (e && t)
	{
		printf("at most one parameter can be dedicated.\n");
		return 0;
	}
	if (!yyin)
	{
		printf("no valid input file!\n");
		throw -1;
		return 0;
	}
	sysY_AST::init_tables();
	do
	{
		yyparse();
	} while (!feof(yyin));
	sysY_AST::check_main();
	if (has_err)
		return 1;
	if (e) // generate eeyore
	{
		auto out_f = fopen(output_s.c_str(), "w");
		fprintf(out_f, "%s", sysY_AST::root->code.c_str());
		return 0;
	}
	else if (v) // debug
	{
		auto out_f = fopen("eeyore.out", "w");
		fprintf(out_f, "%s", sysY_AST::root->code.c_str());
		fflush(out_f);
	}
	// generate tigger
	eeyore_AST::build_AST(sysY_AST::root->code);
	no_alloc::gen_code();
	if (t)
	{
		auto out_f = fopen(output_s.c_str(), "w");
		fprintf(out_f, "%s", tigger_dst.str().c_str());
		return 0;
	}
	else if (v)
	{
		auto out_f = fopen("tigger.out", "w");
		fprintf(out_f, "%s", tigger_dst.str().c_str());
		fflush(out_f);
	}
	// generate risc-v code
	riscv::gen_code();
	
	string ret =
		"\t.text\n\t.align\t2\n\t.global\tmain\n\t.type\tmain, "
		"@function\nmain:\n\tli\ta0, 0\n\tret\n\t.size\tmain, .-main\n";
	if (!(e || t))
	{
		auto out_f = fopen(output_s.c_str(), "w");
		fprintf(out_f, "%s", ret.c_str());
		// fprintf(out_f, "%s", riscv_dst.str().c_str());
		return 0;
	}
	return 0;
}