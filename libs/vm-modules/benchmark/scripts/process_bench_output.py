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

def plot_exec_time_charge(csv_file_path):
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
  plt.savefig('time_charge.png', dpi=100)


def print_exec_time_charge_corr(csv_file_path):
  df = pd.read_csv(csv_file_path)

  print(df['cpu_time'].corr(df['charge']))
  print(df['charge'].divide(df['cpu_time']).mean())
  print(df['charge'].divide(df['cpu_time']))

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

def parse_command_line():
  parser = argparse.ArgumentParser(description='Plot GBench output for VM ML operations charge calculation')
  
  parser.add_argument('csv_file', type=str, help='Location of GBench output csv file')
  
  return parser.parse_args()

def main():
  args = parse_command_line()

  #return plot_vmmodel_charge(args.csv_file)
  #return print_exec_time_charge_corr(args.csv_file)
  csv_file = new_gbench_file_wo_system_info(args.csv_file)
  print_exec_time_charge_corr(csv_file)
  plot_exec_time_charge(csv_file)

if __name__ == '__main__':
  main()
