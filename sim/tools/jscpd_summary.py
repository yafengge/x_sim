#!/usr/bin/env python3
"""
Generate a human-readable summary from jscpd JSON reports.
Writes `reports/jscpd/summary.md` containing top-level stats and file-ranked duplicates.
"""
import json
import glob
import os
from collections import defaultdict, Counter


def load_reports(reports_dir):
    js_files = glob.glob(os.path.join(reports_dir, "*.json"))
    duplicates = []
    for f in js_files:
        try:
            j = json.load(open(f, 'r'))
            # jscpd report may place duplicates under 'duplicates' or 'clones'
            d = j.get('duplicates') or j.get('clones')
            if d:
                duplicates.extend(d)
        except Exception:
            continue
    return duplicates


def summarize(duplicates):
    summary = {}
    summary['duplicate_groups'] = len(duplicates)
    total_dup_lines = 0
    file_counts = Counter()
    for dup in duplicates:
        # jscpd fields vary by version; try several heuristics
        lines = dup.get('lines') or dup.get('fragment') and dup.get('fragment').count('\n') + 1 or 0
        total_dup_lines += lines
        instances = dup.get('instances') or dup.get('occurrences') or []
        for inst in instances:
            fname = inst.get('filename') or inst.get('name') or inst.get('firstFile') or inst.get('file')
            if isinstance(fname, dict):
                fname = fname.get('name')
            if fname:
                file_counts[fname] += 1

    summary['total_duplicated_lines'] = total_dup_lines
    summary['top_files'] = file_counts.most_common(30)
    return summary


def write_md(summary, out_path):
    with open(out_path, 'w') as f:
        f.write('# jscpd Duplicate Report Summary\n\n')
        f.write(f'- Duplicate groups: {summary["duplicate_groups"]}\\n')
        f.write(f'- Total duplicated lines (approx): {summary["total_duplicated_lines"]}\\n\n')
        f.write('## Top files by duplicate occurrences\n\n')
        if not summary['top_files']:
            f.write('No duplicates found.\n')
            return
        f.write('| Count | File |
|---:|---|
')
        for cnt, fname in summary['top_files']:
            f.write(f'| {cnt} | `{fname}` |\n')


def main():
    reports_dir = os.path.join('reports', 'jscpd')
    os.makedirs(reports_dir, exist_ok=True)
    duplicates = load_reports(reports_dir)
    summary = summarize(duplicates)
    out_path = os.path.join(reports_dir, 'summary.md')
    write_md(summary, out_path)
    print('Wrote', out_path)


if __name__ == '__main__':
    main()
