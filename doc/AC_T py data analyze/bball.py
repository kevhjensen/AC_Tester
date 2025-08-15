import pyzipper
import re
import shutil
import loki_agent

FINISHED_FOLDER = "finished_files/" #password unlocked and extracted
folder = r"C:\Users\kevinj\Documents\Log_Man\finished_files\[2025.06]Dispenser1Log\system\Storage\SystemLog"

loki_agent.extract_and_delete_og(folder)