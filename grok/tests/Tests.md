
The test cases cover the four provided examples plus one additional complex case. Each test includes a JSON input and expected CSV outputs.

#### Test 1: Flat Object (`tests/test1.json`)
```json
{"id": 1, "name": "Ali", "age": 19}
```
**Expected**: `tests/expected/test1/people.csv`
```
id,name,age
1,Ali,19
```

#### Test 2: Array of Scalars (`tests/test2.json`)
```json
{
    "movie": "Inception",
    "genres": ["Action", "Sci-Fi", "Thriller"]
}
```
**Expected**:
- `tests/expected/test2/movies.csv`
  ```
  id,movie
  1,Inception
  ```
- `tests/expected/test2/genres.csv`
  ```
  parent_id,index,value
  1,0,Action
  1,1,Sci-Fi
  1,2,Thriller
  ```

#### Test 3: Array of Objects (`tests/test3.json`)
```json
{
    "orderId": 7,
    "items": [
        {"sku": "X1", "qty": 2},
        {"sku": "Y9", "qty": 1}
    ]
}
```
**Expected**:
- `tests/expected/test3/orders.csv`
  ```
  id,orderId
  1,7
  ```
- `tests/expected/test3/items.csv`
  ```
  parent_id,seq,sku,qty
  1,0,X1,2
  1,1,Y9,1
  ```

#### Test 4: Nested Objects + Reused Shape (`tests/test4.json`)
```json
{
    "postId": 101,
    "author": {"uid": "u1", "name": "Sara"},
    "comments": [
        {"uid": "u2", "text": "Nice!"},
        {"uid": "u3", "text": "+1"}
    ]
}
```
**Expected**:
- `tests/expected/test4/posts.csv`
  ```
  id,postId,author_id
  1,101,2
  ```
- `tests/expected/test4/users.csv`
  ```
  id,uid,name
  2,u1,Sara
  3,u2,
  4,u3,
  ```
- `tests/expected/test4/comments.csv`
  ```
  parent_id,seq,uid,text
  1,0,u2,"Nice!"
  1,1,u3,"+1"
  ```

#### Test 5: Complex Nested JSON (`tests/test5.json`)
```json
{
    "company": "TechCorp",
    "departments": [
        {
            "name": "Engineering",
            "employees": [
                {"eid": "e1", "role": "Developer"},
                {"eid": "e2", "role": "Manager"}
            ],
            "projects": ["P1", "P2"]
        },
        {
            "name": "HR",
            "employees": [
                {"eid": "e3", "role": "Recruiter"}
            ],
            "projects": []
        }
    ]
}
```
**Expected**:
- `tests/expected/test5/root.csv`
  ```
  id,company
  1,TechCorp
  ```
- `tests/expected/test5/departments.csv`
  ```
  parent_id,seq,name
  1,0,Engineering
  1,1,HR
  ```
- `tests/expected/test5/employees.csv`
  ```
  parent_id,seq,eid,role
  2,0,e1,Developer
  2,1,e2,Manager
  3,0,e3,Recruiter
  ```
- `tests/expected/test5/projects.csv`
  ```
  parent_id,index,value
  2,0,P1
  2,1,P2
  ```