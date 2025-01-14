#!/usr/bin/bash
line_no=1
inputfile=$2
flag=0
validZips=("zip" "rar" "tar")
otherZip=("tar.gz" "7z" "gz" "bz2" "xz")
languageType=("c" "cpp" "python" "sh")
validLanguages=("c" "cpp" "py" "sh")


matchOutput() {
    
    lineOfOutput=()
    lineofOutput2=()
    penalty=0
    penaltyForUnmatched=5  # Define the penalty per unmatched line, adjust this as needed

    # Read the first file (expected output) and store lines in an array
    while IFS= read -r line; do
        trimmedLine=$(echo "$line" | sed -e 's/^[ \t\r]*//' -e 's/[ \t\r]*$//')
        lineOfOutput+=("$trimmedLine")
    done < "$1"

    while IFS= read -r line; do
        trimmedLine=$(echo "$line" | sed -e 's/^[ \t\r]*//' -e 's/[ \t\r]*$//')
        lineOfOutput2+=("$trimmedLine")
    done < "$2"
    for entry in "${lineOfOutput[@]}"; do
        found="false"
         for entry2 in "${lineOfOutput2[@]}"; do
            if [ "$entry2" = "$entry" ]; then
                found="true"
                break
            fi
        done
        if [ $found = "false" ]; then
             penalty=$((1 + penalty))
        fi  
    done
    echo "$penalty"

}

while read line
do
    trimmedLine=$(echo "$line" | sed -e 's/^[ \t\r]*//' -e 's/[ \t\r]*$//')
    line=$trimmedLine
    if [ $line_no -eq 1 ]; then
        archived=$line
    elif [ $line_no -eq 2 ]; then
        IFS=' ' read -r -a zip_types <<< "$line"   
    elif [ $line_no -eq 3 ]; then
        IFS=' ' read -r -a Languages <<< "$line"
    elif [ $line_no -eq 4 ]; then    
        total_marks=$line
    elif [ $line_no -eq 5 ]; then
        penalty_marks=$line
    elif [ $line_no -eq 6 ]; then
        directory=$line 
    elif [ $line_no -eq 7 ]; then       
        IFS=' ' read -r -a id_range <<< "$line"
    elif [ $line_no -eq 8 ]; then    
        output_path=$line
    elif [ $line_no -eq 9 ]; then
        submission_penalty=$line
    elif [ $line_no -eq 10 ]; then
        plagiarism_path=$line
    elif [ $line_no -eq 11 ]; then
        plagiarism_penalty=$line
    
    fi
    ((line_no++))
done < "$inputfile"

languages=()
for word in ${Languages[@]}
do
    if [[ "$word" == "python" ]]; then 
        languages+=("py")
    else
        languages+=("$word")
    fi
done

if [ $line_no -ne 12 ]; then
    echo "invalid file"
    exit 1
fi

if [[ "$archived" != "true" && "$archived" != "false" ]]; then
    echo "invalid info"
    exit 1
fi
if [[ "$archived" == "true" ]]; then
    for type in ${zip_types[@]}
    do
        if [[ ! " ${validZips[@]} " =~ $type ]]; then
            echo "Invalid zip type"
            exit 1
        fi
    done    
fi        

for type in ${languages[@]}
do
    if [[ ! " ${languageType[@]} " =~ $type ]]; then
        echo "Invalid language type"
        exit 1
    fi   
done

if [[ ! $total_marks =~ ^[0-9]+$ ]]; then
    echo "Not a number"
    exit 1;
fi    
if [[ ! $penalty_marks =~ ^[0-9]+$ ]]; then
    echo "Not a number"
    exit 1;
fi 
if [ ! -d "$directory" ]; then
    echo "Invalid Directory"
fi
if [[ ! ${id_range[0]} =~ ^[0-9]+$ ]]; then
    echo "Invalid id"
    exit 1
elif [[ ! ${id_range[1]} =~ ^[0-9]+$ ]]; then
    echo "Invalid id"
    exit 1
fi
if [ ! -f "$output_path" ]; then
    echo "Invalid output path"
    exit 1
fi

if [[ ! $submission_penalty =~ ^[0-9]+$ ]]; then
    echo "Not a number"
    exit 1;
fi
if [ ! -f "$plagiarism_path" ]; then
    echo $plagiarism_path
    echo "Invalid plagiarism path"
    exit 1
fi
if [[ ! $plagiarism_penalty =~ ^[0-9]+$ ]]; then
    echo $plagiarism_penalty
    echo "plagiarism penalty Not a number"
    exit 1;
fi

mkdir "$directory/arch"

declare -A wasArchieved
declare -A unmatched
declare -A evaluated

#umatched["21"]=123
echo "id,marks,marks_deducted,total_marks,remarks" > "$directory/marks.csv"


mkdir "$directory/checked"
mkdir "$directory/issues"
checked="$directory/checked"
issues="$directory/issues"

folder="false"

declare -A plagiarism_ids
while read -r plag_id; do
    plagiarism_ids[$plag_id]=1
done < "$plagiarism_path"

x=0 y=0 z=$total_marks

for file in "$directory"/*;
do
    remarks=""
    marks_deducted=0
    if [ -f "$file" ]; then
        filename=$(basename "$file")
        id=${filename%%.*}
        parent=$id
        file_extension="${file##*.}"
        #if [[ "$file" =~ \.zip$ || "$file" =~ \.rar$ || "$file" =~ \.tar ]]; then
        if [[ " $id " =~ ^[0-9]+$ ]]; then
            echo "$id 1"
            continue
        elif (( !$id >= ${id_range[0]} && !$id <= ${id_range[1]} )); then
            continue
        elif [[  " ${zip_types[@]} " =~ " $file_extension " ]]; then
            wasArchieved[$id]="true";
            echo "$file"
            if [[ "$file" =~ \.zip$ ]]; then
                unzip "$file" -d "$directory/arch"
            elif [[ "$file" =~ \.rar$ ]]; then
                unrar x "$file" "$directory/arch"
            elif [[ "$file" =~ \.tar$ ]]; then
                tar -xf "$file" -C "$directory/arch"    
            fi
        elif [[  " ${otherZip[@]} " =~ " $file_extension " ]]; then
            remarks="Issue Case#2"
            
            evaluated[$id]="true"
            y=$submission_penalty
            z=-$y
            echo "$id,$x,$y,$z,\"$remarks\"" >> "$directory/marks.csv"  
            continue
        elif [[ ! "$file" =~ \.txt$ && ! "$file" =~ \.csv$ || !"$id" =~ "checked" || !"$id" =~ "issues" ]]; then
            mkdir -p "$directory/arch/$id"
            mv "$file" "$directory/arch/$id/"    
        
        fi
    elif [ -d "$file" ]; then
        filename=$(basename "$file")
        id=${filename%%.*}
        parent=$id
        if [[ " $id " =~ ^[0-9]+$ ]]; then
            continue
        elif (( !$id >= ${id_range[0]} && !$id <= ${id_range[1]} )); then
            #remarks="Issue case#5"
            #echo "$i,$x,$y,$x,\"$remarks\"" >> "$directory/marks.csv"
            continue
        elif [[ " $id " =~ "arch" ]]; then
            continue;
        elif [[ "$id" =~ "checked" || "$id" =~ "issues" ]]; then
            continue
        fi
        folder="true"
        marks_deducted=$((marks_deducted + submission_penalty))
        remarks+="Issue case#1"
        cp -r "$file" "$directory/arch/$id"
        rm -rf "$file" 
    fi
    for file in "$directory/arch"/*;
    do
        valid_submission=false
        filename3=$(basename "$file")
        child=${filename3%%.*}
        if [ -d "$file" ]; then
            # remarks=""
            # marks_deducted=0
            attainedMarks=$total_marks        
            for exFile in "$file"/*;
            do
                filename=$(basename "$exFile")
                id=${filename%%.*}
                codeName=$id
                file_extension="${exFile##*.}"
                if [[  " ${languages[@]} " =~ " $file_extension " ]]; then
                    valid_submission=true;    
                fi        
                break
            done
            if [ "$valid_submission" = false ]; then
                cp -r "$file" "$issues"
                rm -rf "$file"
                attainedMarks=0
                remarks+=" Issue case #3:"
                evaluated[$id]="true"
                y=$submission_penalty
                z=-$y
                echo "$id,$x,$y,$z,\"$remarks\"" >> "$directory/marks.csv" 
                continue
            elif [[ ! $parent =~ $codeName ]]; then
                remarks+="Issue case#3"
                y=$submission_penalty
                z=-$y
                echo "$id,$x,$y,$z,\"$remarks\"" >> "$directory/marks.csv" 
            else 
                # echo "Running student's program..."
                case "$file_extension" in
                    c)
                        gcc "$exFile" -o "$file/$id.out"
                        "$file/$id.out" > "$file/${id}_output.txt"
                        ;;
                    cpp)
                        g++ "$exFile" -o "$file/$id.out"
                        "$file/$id.out" > "$file/${id}_output.txt"
                        ;;
                    py)
                        python3 "$exFile" > "$file/${id}_output.txt"
                        ;;
                    sh)
                        bash "$exFile" > "$file/${id}_output.txt"
                        ;;
                esac
            fi
            #missing_lines=0
            # while read -r expected_line; do
            #     if ! grep -qF "$expected_line" "$file/${id}_output.txt"; then
            #         ((missing_lines++))
            #     fi
            # done < "$output_path"
            missing_lines=$(matchOutput "$output_path" "$file/${id}_output.txt" )

            # echo "$missing_lines" "----" $id

            ##marks_deducted=$((marks_deducted + missing_lines * penalty_marks))
            attainedMarks=$((attainedMarks - missing_lines * penalty_marks))

            if [[ " $archived " =~ "false" ]] || [[ " $folder " =~ "true" ]]; then
                #do nothing
                abc=1
            elif [[ ! ${wasArchieved[$id]} =~ $archived ]]; then
                marks_deducted=$((marks_deducted + submission_penalty))
                remarks+="Issue case#1"   #check issue type
            fi
            if [[ ! $parent =~ $child ]]; then
                marks_deducted=$((marks_deducted + submission_penalty))
                remarks+="Issue case#4" 
            fi
            total_marks_earned=$((attainedMarks - marks_deducted))
            
            if [[ -n "${plagiarism_ids[$id]}" ]]; then
                remarks+=" Plagiarism detected"
                total_marks_earned=-$total_marks
                # plagiarism_penalty_amount=$total_marks
                # marks_deducted=$((marks_deducted + plagiarism_penalty_amount))
            fi
            evaluated[$id]="true"
            echo "$id,$attainedMarks,$marks_deducted,$total_marks_earned,\"$remarks\"" >> "$directory/marks.csv"
            cp -r "$file" "$checked"
            rm -rf "$file" 
        fi
    done
done         

rm -rf "$directory/arch"


for i in $(seq ${id_range[0]} ${id_range[1]}); do
    remarks="Not submitted"
    if [[ ! " ${evaluated[$i]} " =~ "true" ]]; then
        echo "$i,$x,$y,$x,\"$remarks\"" >> "$directory/marks.csv"
    fi
done

