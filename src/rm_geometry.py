import re
import os

geo = r'<property name=\"geometry\">.*?</property>[ \n]+'

for fnam in os.listdir('../src/'):
    if fnam.endswith('.ui'):
        with open(fnam, 'r') as f:
            tin = f.read()
            tout = re.sub(geo, '', tin, flags=re.MULTILINE|re.DOTALL)
        if tin != tout:
            with open(fnam, 'w') as f:
                f.write(tout)


