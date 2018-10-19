# import pstats
# p = pstats.Stats('profile_stats.txt')
# # p.strip_dirs().sort_stats(-1).print_stats()
#
# p.sort_stats('cumtime').print_stats()

from mlearner import MLearner
from line_profiler import LineProfiler

# try:
#     from line_profiler import LineProfiler
#
#     def do_profile(follow=[]):
#         def inner(func):
#             def profiled_func(*args, **kwargs):
#                 try:
#                     profiler = LineProfiler()
#                     profiler.add_function(func)
#                     for f in follow:
#                         profiler.add_function(f)
#                     profiler.enable_by_count()
#                     return func(*args, **kwargs)
#                 finally:
#                     profiler.print_stats()
#             return profiled_func
#         return inner
#
# except ImportError:
#     def do_profile(follow=[]):
#         "Helpful if you accidentally leave in production!"
#         def inner(func):
#             def nothing(*args, **kwargs):
#                 return func(*args, **kwargs)
#             return nothing
#         return inner

# @do_profile()
def run_mnist():

    mlearner = MLearner()

    # load the data
    mlearner.load_data(one_hot=True)

    # being training
    mlearner.train()


run_mnist()
#
# lp = LineProfiler()
# lp_wrapper = lp(run_mnist())
# # lp_wrapper()
# lp.print_stats()