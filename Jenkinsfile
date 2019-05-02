pipeline {

  agent none

  stages {

    stage('Basic Checks') {
      agent {
        docker {
          image "gcr.io/organic-storm-201412/fetch-ledger-develop:latest"

        }
      }

      stages {
        stage('License Checks') {
          steps {
            sh './scripts/check_license_header.py'
          }
        }
        stage('Style check') {
          steps {
            sh './scripts/apply_style.py -w -a'
          }
        }
        stage('CMake Version Check') {
          steps {
            sh './scripts/check-cmake-versions.py'
          }
        }
      }
    } // basic checks

    stage('Builds & Tests') {
      parallel {

        stage('Static Analysis') {
          agent {
            docker {
              image "gcr.io/organic-storm-201412/fetch-ledger-develop:latest"
              label "ledger"
            }
          }

          stages {
            stage('Static Analysis') {
              steps {
                sh 'mkdir -p build-analysis && cd build-analysis && cmake ../'
                sh './scripts/run_static_analysis.py build-analysis/'
              }
            }
          }
        } // static analysis

        stage('Clang 6 Debug') {
          agent {
            docker {
              image "gcr.io/organic-storm-201412/fetch-ledger-develop:latest"
              label "ledger"
            }
          }

          stages {
            stage('Debug Build') {
              steps {
                sh './scripts/ci-tool.py -B Debug'
              }
            }

            stage('Debug Unit Tests') {
              steps {
                sh './scripts/ci-tool.py -T Debug'
              }
            }

            stage('Debug Integration Tests') {
              when {
                branch "develop"
              }
              steps {
                sh './scripts/ci-tool.py -I Debug'
              }
            }
          }
        } // clang 6 debug

        stage('Clang 6 Release') {
          agent {
            docker {
              image "gcr.io/organic-storm-201412/fetch-ledger-develop:latest"
              label "ledger"
            }
          }

          stages {
            stage('Release Build') {
              steps {
                sh './scripts/ci-tool.py -B Release'
              }
            }

            stage('Unit Tests') {
              steps {
                sh './scripts/ci-tool.py -T Release'
              }
            }

            stage('Integration Tests') {
              when {
                branch "develop"
              }
              steps {
                sh './scripts/ci-tool.py -I Release'
              }
            }
          }
        } // clang 6 release

        stage('GCC 7 Debug') {
          agent {
            docker {
              image "gcr.io/organic-storm-201412/fetch-ledger-develop:latest"
              label "ledger"
            }
          }

          environment {
            CC  = 'gcc'
            CXX = 'g++'
          }

          stages {
            stage('GCC Debug Build') {
              steps {
                sh './scripts/ci-tool.py -B Debug'
              }
            }

            stage('GCC Debug Unit Tests') {
              steps {
                sh './scripts/ci-tool.py -T Debug'
              }
            }

            stage('GCC Debug Integration Tests') {
              when {
                branch "develop"
              }
              steps {
                sh './scripts/ci-tool.py -I Debug'
              }
            }
          }
        } // gcc 7 debug

        stage('GCC 7 Release') {
          agent {
            docker {
              image "gcr.io/organic-storm-201412/fetch-ledger-develop:latest"
              label "ledger"
            }
          }

          environment {
            CC  = 'gcc'
            CXX = 'g++'
          }

          stages {
            stage('GCC Release Build') {
              steps {
                sh './scripts/ci-tool.py -B Release'
              }
            }

            stage('GCC Release Unit Tests') {
              steps {
                sh './scripts/ci-tool.py -T Release'
              }
            }

            stage('GCC Release Integration Tests') {
              when {
                branch "develop"
              }
              steps {
                sh './scripts/ci-tool.py -I Release'
              }
            }
          }
        } // gcc 7 release

      } // parallel

    } // build & test

  } // stages

} // pipeline
