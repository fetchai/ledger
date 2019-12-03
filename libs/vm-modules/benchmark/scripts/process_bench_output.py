import os
import argparse
import matplotlib.pyplot as plt
import pandas as pd
import csv


def plot_vmmodel_charge(csv_file_path):
  time = []
  charge = []
  
  with open(csv_file_path, 'r') as csvfile:
    data = csv.reader(csvfile, delimiter=',')
    next(data, None)
    for row in data:
      time.append(float(row[3]))  
      charge.append(int(row[10]))

  plot.plot(time)
  plot.show()
  plot.plot(charge)
  plot.show()

def plot_exec_time_charge(csv_file_path, root):
  df = pd.read_csv(csv_file_path)
  
  figure = plt.figure(figsize=(8,6), dpi=100)
  axe = figure.add_subplot(111)

  axe.plot(df['cpu_time'])
  axe.set_ylabel('cpu_time')
  axe.set_yscale("log")
  axe.set_xticklabels(df['name'], rotation='vertical')

  axe2 = axe.twinx()
  axe2.plot(df['charge'], '-r')
  axe2.set_ylabel('charge_amount', color='r')
  axe2.set_yscale("log")

  plt.tight_layout()
  name = os.path.join(root, os.path.splitext(csv_file_path)[0]+"_time_charge.png")
  plt.savefig(name)


def print_exec_time_charge_corr(csv_file_path, root):
  df = pd.read_csv(csv_file_path)
  
  correlation_coef = df['cpu_time'].corr(df['charge'])
  time_charge_factors = df['charge'].divide(df['cpu_time'])
  time_charge_factor_avg = time_charge_factors.mean()
  
  aggregate_stats = os.path.join(root, os.path.splitext(csv_file_path)[0]+"_stats")
  with open(aggregate_stats, 'w') as out:
    print(correlation_coef)
    out.write(str(correlation_coef)+os.linesep)
    print(time_charge_factor_avg)
    out.write(str(time_charge_factor_avg)+os.linesep)

  factors = os.path.join(root, os.path.splitext(csv_file_path)[0]+"_factors")
  with open(factors, 'w') as out:
    print(time_charge_factors)
    out.writelines(str(time_charge_factors))

def new_gbench_file_wo_system_info(original_csv_file):
  # TOFIX should use os join
  processed_csv_file = os.path.splitext(original_csv_file)[0]+"_wo_sysinfo.csv"
  with open(original_csv_file, 'r') as original:
    lines = original.read().splitlines(True)
  with open(processed_csv_file, 'w') as processed:
    counter = 0
    for line in lines:
      if line[0:4] == "name":
        processed.writelines(lines[counter:])
        break
      #print('skipping line {}'.format(line))
      counter += 1
  return processed_csv_file

def process_bench(csvfile, root):
  print_exec_time_charge_corr(csvfile, root)
  plot_exec_time_charge(csvfile, root)
  

def parse_command_line():
  parser = argparse.ArgumentParser(description='Plot GBench output for VM ML operations charge calculation')
  
  parser.add_argument('csv_file', type=str, help='Location of GBench output csv file')
  parser.add_argument('output_dir', type=str, help='Output directory')

  return parser.parse_args()

def main():
  args = parse_command_line()

  csvfile = new_gbench_file_wo_system_info(args.csv_file)
  root = args.output_dir

  process_bench(csvfile, root)

if __name__ == '__main__':
  main()
