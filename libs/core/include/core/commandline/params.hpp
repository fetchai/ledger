#ifndef PARAMS_HPP
#define PARAMS_HPP

#include <functional>
#include <iostream>
#include <string>
#include <list>
#include <tuple>
#include <set>
#include <stdexcept>

#include "core/commandline/parameter_parser.hpp"

namespace fetch {
namespace commandline {

    class Params
    {
    public:
        Params(const Params &rhs)           = delete;
        Params(Params &&rhs)           = delete;
        Params operator=(const Params &rhs)  = delete;
        Params operator=(Params &&rhs) = delete;
        bool operator==(const Params &rhs) const = delete;
        bool operator<(const Params &rhs) const = delete;

        typedef std::function<
            void(const std::set<std::string> &, std::list<std::string> &)
        > action_func_type;
        typedef std::tuple<std::string, std::string> help_text_type;
        typedef std::map<std::string, action_func_type> assigners_type;
        typedef std::list<help_text_type> help_texts_type;

        Params():paramsParser_()
        {
        }

        void Parse(int argc, const char *argv[])
        {
            paramsParser_.Parse(argc, argv);

            std::list<std::string> errs;
            std::set<std::string> args;

            for(int i=0;i<argc;i++)
            {
                auto s = std::string(argv[i]);
                if (s.size()>0 && s[0] == '-')
                {
                    if (s.size()>1 && s[1] == '-')
                    {
                        args.insert(s.substr(2));
                    }
                    else
                    {
                        args.insert(s.substr(1));
                    }
                }
            }

            for(auto action : assigners_)
            {
                action_func_type func = action.second;
                func(args, errs);
            }

            for(int i = 0;i< argc;i++)
            {
                std::string arg(argv[i]);
                if (arg == "--help" || arg == "-h")
                {
                    help();
                    exit(0);
                }
            }

            if (!errs.empty())
            {
                for(auto err : errs)
                {
                    std::cerr << err << std::endl;
                }
                exit(1);
            }
        }


        virtual ~Params()
        {
        }

        template<class TYPE>
        void add(TYPE &assignee,
                 const std::string &name,
                 const std::string &help,
                 TYPE deflt)
        {
            std::string name_local(name);
            TYPE deflt_local = deflt;
            assigners_[name] = [name_local,&assignee,deflt_local,this](
                const std::set<std::string> &args,
                std::list<std::string> &errs) mutable
                {
                    assignee = this -> paramsParser_.GetParam<TYPE>(
                        name_local, deflt_local);
                };

            helpTexts_.push_back(help_text_type{name, help});
        }

        template<class TYPE>
        void add(TYPE &assignee,
                 const std::string &name,
                 const std::string &help)
        {
            std::string name_local(name);
            assigners_[name] =
                [name_local,&assignee,this](
                    const std::set<std::string> &args,
                    std::list<std::string> &errs) mutable
                {
                    if (args.find(name_local) == args.end())
                    {
                        errs.push_back("Missing required argument: " + name_local);
                        return;
                    }
                    assignee = this -> paramsParser_.GetParam<TYPE>(
                        name_local, TYPE());
                };

            helpTexts_.push_back(help_text_type{name, help});
        }

        void description(const std::string &desc)
        {
            desc_ = desc;
        }

        void help()
        {
            if (!desc_.empty())
            {
                std::cerr << desc_ << '\n' << std::endl;
            }

            size_t w = 0;

            for(auto helpText : helpTexts_)
            {
                auto length = std::get<0>(helpText).length();
                if (length> w)
                {
                    w = length;
                }
            }

            for(auto helpText : helpTexts_)
            {
                auto padsize = w - std::get<0>(helpText).length();
                std::cerr << "  -"
                          << std::get<0>(helpText)
                          << std::string( 2 + padsize, ' ')
                          << std::get<1>(helpText) << std::endl;
            }
        }
private:
    fetch::commandline::ParamsParser paramsParser_;
    std::string desc_;
    help_texts_type helpTexts_;
    assigners_type assigners_;
};

}
}

#endif //__PARAMS_HPP
