"""
A small script to put all csv's in a directory in a single csv.
"""

from pathlib import Path
import pandas as pd

directory = Path("../data/psi_affiliation")

csv_files = [f for f in directory.glob("*.csv") if f.name != "psi_affiliation.csv"]

if not csv_files:
    print("No CSV files found :(")
else:
    dataframes = []
    for file in csv_files:
        try:
            df = pd.read_csv(file)
            df["source_file"] = file.name 
            dataframes.append(df)
            print(f"Loaded: {file.name} ({len(df)} rows)")
        except Exception as e:
            print(f"Skipped {file.name}: {e}")

    if dataframes:
        merged = pd.concat(dataframes, ignore_index=True, sort=False)
        output_file = directory / "psi_affiliation.csv"
        merged.to_csv(output_file, index=False)

        print("\nSTATS:")
        print(f"Merged {len(dataframes)} files.")
        print(f"Total rows: {len(merged)}")
        print(f"Output: {output_file}")