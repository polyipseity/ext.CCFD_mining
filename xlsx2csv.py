import pandas as pd


def xlsx_to_csv_pd(filepath):
    data_xls = pd.read_excel(filepath, index_col=0)
    csv_path = filepath.split('.')[0] + '.csv'
    data_xls.to_csv(csv_path, encoding='utf-8')


if __name__ == '__main__':
    filepath = "table_mock.xlsx"
    xlsx_to_csv_pd(filepath)
